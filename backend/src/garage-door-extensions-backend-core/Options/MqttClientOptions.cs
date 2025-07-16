public class MqttClientOptions
{
    public string BrokerAddress { get; set; }
    public int Port { get; set; } = 1883;
    public string ClientId { get; set; } = Guid.NewGuid().ToString();
    public bool CleanSession { get; set; } = true;
    public string Username { get; set; } = string.Empty;
    public string Password { get; set; } = string.Empty;
}