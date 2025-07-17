using BlogEivindGLCom.GarageDoorExtensionsBackend.Service;

var builder = Host.CreateApplicationBuilder(args);
builder.Services.AddHostedService<Worker>();
builder.Services.Configure<MqttClientOptions>(builder.Configuration.GetSection("MqttClientOptions"));
builder.Services.Configure<AlertOptions>(builder.Configuration.GetSection("AlertOptions"));
builder.Services.AddHttpClient(Worker.BackendApiHttpClient, client =>
{
    var configSection = builder.Configuration.GetSection("BackendApi");
    client.BaseAddress = new Uri(configSection.GetValue<string>("BaseUri") ?? "http://localhost:80");
    client.Timeout = TimeSpan.FromSeconds(5);
});

var host = builder.Build();
host.Run();
