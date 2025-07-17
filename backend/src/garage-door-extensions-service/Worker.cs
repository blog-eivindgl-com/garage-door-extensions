using System.Net;
using System.Runtime.CompilerServices;
using System.Text;
using Microsoft.Extensions.Options;
using MQTTnet;

namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Service;

public class Worker : BackgroundService
{
    internal const string BackendApiHttpClient = "BackendApi";
    private const string GarageDoorStateChangedTopic = "garageDoor/doorStateChanged";
    private const string GarageDoorDisplayTopic = "garageDoor/display";
    private const string AlertTopic = "garageDoor/alert";
    private const string AlertMessageDurationPlaceholder = "{duration}";
    private const string OpeningState = "opening";
    private const string ClosedState = "closed";
    private const string OpeningApiEndpoint = "api/dooropenings/RegisterDoorOpening";
    private const string ClosedApiEndpoint = "api/dooropenings/RegisterDoorClosing";
    private const string DisplayApiEndpoint = "api/dooropenings/display";
    private readonly MQTTnet.MqttClientOptions _mqttClientOptions;
    private readonly MQTTnet.MqttClientSubscribeOptions _mqttDoorStateSubscribeOptions;
    private IMqttClient _mqttClient;
    private readonly IOptions<AlertOptions> _alertOptions;
    private readonly IHttpClientFactory _httpClientFactory;
    private readonly ILogger<Worker> _logger;
    private DateTime? _lastPing;
    private DateTime? _lastAlertCheck;
    private long _alertSentForDoorOpenedAt = 0; // Timestamp of the time that the door was opened for the last alert sent. Used to prevent multiple alerts for the same door open event.
    private bool disposed = false;

    public Worker(IOptions<MqttClientOptions> mqttClientOptions, IOptions<AlertOptions> alertOptions, IHttpClientFactory httpClientFactory, ILogger<Worker> logger)
    {
        if (mqttClientOptions == null) throw new ArgumentNullException(nameof(mqttClientOptions), "MqttClientOptions cannot be null");
        if (alertOptions == null) throw new ArgumentNullException(nameof(alertOptions), "AlertOptions cannot be null");
        _alertOptions = alertOptions;
        if (httpClientFactory == null) throw new ArgumentNullException(nameof(httpClientFactory), "IHttpClientFactory cannot be null");
        _httpClientFactory = httpClientFactory;
        _logger = logger;

        // Validate AlertOptions
        if (_alertOptions.Value.GarageDoorOpenTimeoutSeconds <= 0)
        {
            _logger.LogWarning("GarageDoorOpenTimeoutSeconds is not set or invalid, disabling alerts.");
        }

        // Validate and set up MQTT client options
        if (string.IsNullOrWhiteSpace(mqttClientOptions.Value.BrokerAddress))
        {
            throw new ArgumentException("BrokerAddress cannot be null or empty", nameof(mqttClientOptions));
        }

        var mqttFactory = new MqttClientFactory();
        _mqttClient = mqttFactory.CreateMqttClient();
        _mqttClientOptions = new MqttClientOptionsBuilder()
            .WithTcpServer(mqttClientOptions.Value.BrokerAddress, port: mqttClientOptions.Value.Port)
            .WithCredentials(mqttClientOptions.Value.Username, mqttClientOptions.Value.Password)
            .Build();
        _mqttClient.ApplicationMessageReceivedAsync += HandleIncomingMessageAsync;
        _mqttDoorStateSubscribeOptions = mqttFactory.CreateSubscribeOptionsBuilder()
            .WithTopicFilter(GarageDoorStateChangedTopic).Build();
    }

    private HttpClient GetHttpClient()
    {
        return _httpClientFactory.CreateClient(BackendApiHttpClient);
    }

    protected override async Task ExecuteAsync(CancellationToken stoppingToken)
    {
        while (!stoppingToken.IsCancellationRequested)
        {
            // Connect to the MQTT broker and keep the connection alive
            await ConnectAndKeepAliveAsync(stoppingToken);

            // Publish MQTT message for failure when the garage door i not closed after a certain time
            await AlertOnDoorNotClosedAsync(stoppingToken);

            // Wait for a while before the next iteration
            await Task.Delay(1000, stoppingToken);
        }

        // Ensure the MQTT client is disposed when the service stops
        Dispose();
    }
    
    private async Task HandleIncomingMessageAsync(MqttApplicationMessageReceivedEventArgs e)
    {
        try
        {
            var payload = Encoding.UTF8.GetString(e.ApplicationMessage.Payload);
            _logger.LogInformation($"Received message on topic {e.ApplicationMessage.Topic}: {payload}");

            if (OpeningState.Equals(payload, StringComparison.OrdinalIgnoreCase))
            {
                _logger.LogInformation("Garage door is opening.");

                using (var cts = new CancellationTokenSource(TimeSpan.FromSeconds(15)))
                {
                    await RegisterDoorStateAsync(OpeningApiEndpoint, cts.Token);
                }

                using (var cts = new CancellationTokenSource(TimeSpan.FromSeconds(15)))
                {
                    await UpdateDisplayCounter(cts.Token);
                }
            }
            else if (ClosedState.Equals(payload, StringComparison.OrdinalIgnoreCase))
            {
                _logger.LogInformation("Garage door is closed.");

                using (var cts = new CancellationTokenSource(TimeSpan.FromSeconds(15)))
                {
                    await RegisterDoorStateAsync(ClosedApiEndpoint, cts.Token);
                }
            }
            else
            {
                _logger.LogWarning($"Unknown state received: {payload}");
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error processing incoming MQTT message");
        }
    }

    private async Task RegisterDoorStateAsync(string relativeUri, CancellationToken stoppingToken)
    {
        try
        {
            using var httpClient = GetHttpClient();
            var response = await httpClient.PostAsync(relativeUri, content: null, stoppingToken);
            response.EnsureSuccessStatusCode();
            _logger.LogInformation($"Successfully registered door state: {relativeUri}");
        }
        catch (Exception ex)
        {
            //  TODO: Use Polly to retry on failure
            _logger.LogError(ex, $"Failed to register door state: {relativeUri} - {ex.Message}");
        }
    }

    private async Task ConnectAndKeepAliveAsync(CancellationToken stoppingToken)
    {
        try
        {
            // Ping every 5 seconds to test the connection and reconnect if necessary
            if (!_lastPing.HasValue || _lastPing.Value.AddSeconds(5) < DateTime.UtcNow)
            {
                _lastPing = DateTime.UtcNow;

                // This code will also do the very first connect! So no call to _ConnectAsync_ is required in the first place.
                if (!await _mqttClient.TryPingAsync(stoppingToken))
                {
                    _logger.LogInformation("MQTT client is not connected, attempting to connect...");
                    var result = await _mqttClient.ConnectAsync(_mqttClientOptions, stoppingToken);

                    if (result.ResultCode != MQTTnet.MqttClientConnectResultCode.Success)
                    {
                        _logger.LogError($"Failed to connect to MQTT broker: {result.ReasonString}");
                        return; // Exit if connection fails
                    }

                    _logger.LogInformation("The MQTT client is connected.");
                    
                    // Subscribe to the MQTT topic garageDoor/doorStateChanged
                    await _mqttClient.SubscribeAsync(_mqttDoorStateSubscribeOptions, stoppingToken);
                    _logger.LogInformation($"Subscribed to topic: {GarageDoorStateChangedTopic}");
                }
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"An error occurred while connecting to the MQTT broker. {ex.Message}");
        }
    }
    
    private async Task UpdateDisplayCounter(CancellationToken stoppingToken)
    {
        try
        {
            // Call API to build MQTT message to update the display counter
            using var httpClient = GetHttpClient();
            var response = await httpClient.GetAsync(DisplayApiEndpoint, stoppingToken);

            if (!response.IsSuccessStatusCode)
            {
                _logger.LogError($"Failed to get display data: {response.ReasonPhrase}");
                return;
            }

            var displayData = await response.Content.ReadAsStringAsync(stoppingToken);

            // Publish an MQTT message to update the display counter
            await _mqttClient.PublishAsync(new MqttApplicationMessageBuilder()
                .WithTopic(GarageDoorDisplayTopic)
                .WithPayload(displayData)
                .WithQualityOfServiceLevel(MQTTnet.Protocol.MqttQualityOfServiceLevel.AtLeastOnce)
                .Build(), stoppingToken);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error updating display counter");
        }
    }

    private async Task AlertOnDoorNotClosedAsync(CancellationToken stoppingToken)
    {
        if (_alertOptions.Value.GarageDoorOpenTimeoutSeconds <= 0)
        {
            return;
        }

        // Check if the last alert check was more than the configured timeout ago
        // This prevents multiple alerts within the timeout period
        // Add one second to ensure we don't miss the alert if the door is open exactly at the timeout
        if (!_lastAlertCheck.HasValue || _lastAlertCheck.Value.AddSeconds(_alertOptions.Value.GarageDoorOpenTimeoutSeconds + 1) < DateTime.UtcNow)
        {
            _lastAlertCheck = DateTime.UtcNow;

            // Check if the door is currently open by comparing last opened and closed times
            // If the door is open, we will send an alert if it has been open for too long
            using var httpClient = GetHttpClient();
            var lastOpenedResponse = await httpClient.GetAsync("api/dooropenings/lastopened", stoppingToken);
            if (!lastOpenedResponse.IsSuccessStatusCode)
            {
                _logger.LogError($"Failed to get last opened time: {lastOpenedResponse.ReasonPhrase}");
                return;
            }
            var lastOpened = await lastOpenedResponse.Content.ReadAsStringAsync(stoppingToken);

            var lastClosedResponse = await httpClient.GetAsync("api/dooropenings/lastclosed", stoppingToken);
            if (!lastClosedResponse.IsSuccessStatusCode)
            {
                _logger.LogError($"Failed to get last closed time: {lastClosedResponse.ReasonPhrase}");
                return;
            }

            var lastClosed = await lastClosedResponse.Content.ReadAsStringAsync(stoppingToken);
            if (string.IsNullOrWhiteSpace(lastOpened) || string.IsNullOrWhiteSpace(lastClosed))
            {
                _logger.LogWarning("No door open or closed events found, skipping alert check.");
                return; // No door open or closed events found, skip alert check
            }

            // If the door was opened after it was closed, we consider it open
            if (long.TryParse(lastOpened, out long lastOpenedTimestamp) && long.TryParse(lastClosed, out long lastClosedTimestamp) && lastClosedTimestamp > lastOpenedTimestamp)
            {
                return; // No alert needed, door is closed
            }

            // Check if the door has been open for too long
            var response = await httpClient.GetAsync("api/dooropenings/openduration", stoppingToken);

            if (!response.IsSuccessStatusCode)
            {
                _logger.LogError($"Failed to get door open duration: {response.ReasonPhrase}");
                return;
            }

            var openDuration = await response.Content.ReadAsStringAsync(stoppingToken);

            if (int.TryParse(openDuration, out int duration) && duration < _alertOptions.Value.GarageDoorOpenTimeoutSeconds)
            {
                _logger.LogInformation($"Garage door has been open for {duration} seconds, which is less than the configured timeout of {_alertOptions.Value.GarageDoorOpenTimeoutSeconds} seconds.");
                return; // No alert needed
            }

            // Check if an alert has already been sent for the last opened time
            if (lastOpenedTimestamp > _alertSentForDoorOpenedAt)
            {
                _alertSentForDoorOpenedAt = lastOpenedTimestamp; // Update the timestamp of the last alert sent
            }
            else
            {
                _logger.LogInformation("No new door open event detected since the last alert.");
                return; // No new door open event, no alert needed
            }

            _logger.LogWarning($"Garage door has been open for {openDuration} seconds, sending alert...");

            // Publish an alert message to the MQTT broker
            string? alertMessageContent = null;

            if (!string.IsNullOrWhiteSpace(_alertOptions.Value.AlertMessage))
            {
                alertMessageContent = _alertOptions.Value.AlertMessage.Replace(AlertMessageDurationPlaceholder, openDuration);
            }

            var alertMessage = new MqttApplicationMessageBuilder()
                .WithTopic(AlertTopic)
                .WithPayload(alertMessageContent ?? openDuration)
                .WithQualityOfServiceLevel(MQTTnet.Protocol.MqttQualityOfServiceLevel.AtLeastOnce)
                .Build();

            await _mqttClient.PublishAsync(alertMessage, stoppingToken);
        }
    }

    // Protected Dispose method (core cleanup logic)
    protected virtual void Dispose(bool disposing)
    {
        if (!disposed)
        {
            if (disposing)
            {
                // Dispose managed resources
                if (_mqttClient != null)
                {
                    try
                    {
                        _logger.LogInformation("Disconnecting MQTT client...");

                        // This will send the DISCONNECT packet. Calling _Dispose_ without DisconnectAsync the
                        // connection is closed in a "not clean" way. See MQTT specification for more details.
                        using (var cts = new CancellationTokenSource(TimeSpan.FromSeconds(5)))
                        {
                            _mqttClient.DisconnectAsync(
                                new MqttClientDisconnectOptionsBuilder()
                                    .WithReason(MqttClientDisconnectOptionsReason.NormalDisconnection)
                                    .Build(),
                                cancellationToken: cts.Token).GetAwaiter().GetResult();
                        }
                    }
                    catch (Exception ex)
                    {
                        _logger.LogError(ex, "Failed to disconnect MQTT client gracefully.");
                    }

                    _mqttClient?.Dispose();
                }
            }

            // Free unmanaged resources
            _mqttClient = null!; // Using null-forgiving operator since we're disposing
            disposed = true;
        }
    }

    // Finalizer (called by the garbage collector)
/*     ~Worker()
    {
        Dispose(false); // Dispose only unmanaged resources
    } */
    
    
    // Public Dispose method (called explicitly)
    public override void Dispose()
    {
        Dispose(true); // Dispose managed and unmanaged resources
        base.Dispose(); // Call base Dispose to clean up the base class
        GC.SuppressFinalize(this); // Suppress finalization
    }
}
