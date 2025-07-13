using BlogEivindGLCom.GarageDoorExtensionsBackend.Services;
using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Routing;

var builder = WebApplication.CreateBuilder(args);

// Configure Kestrel for HTTPS in development
if (builder.Environment.IsDevelopment())
{
    builder.WebHost.ConfigureKestrel(options =>
    {
        options.ListenLocalhost(5035); // HTTP
        options.ListenLocalhost(7058, listenOptions =>
        {
            listenOptions.UseHttps(); // HTTPS
        });
    });
}

// Add services to the container.
builder.Services.AddSingleton<IDateTimeService, DateTimeService>();
var doorOpeningServiceType = builder.Configuration.GetValue<string>("DoorOpeningsService:Type");
switch (doorOpeningServiceType)
{
    case "FileSystem":
        builder.Services.Configure<DoorOpeningsFileSystemOptions>(builder.Configuration.GetSection("DoorOpeningsService"));
        builder.Services.AddSingleton<IDoorOpeningsService, DoorOpeningsFileSystemService>();
        break;
    case "Dummy":
        builder.Services.AddSingleton<IDoorOpeningsService, DoorOpeningsDummyService>();
        break;
    default:
        throw new InvalidOperationException("Invalid DoorOpeningsService type configured");
}

builder.Services.AddSingleton<IDoorOpeningsService, DoorOpeningsFileSystemService>();
builder.Services.AddControllers(); // This supports attribute routing better than AddControllersWithViews for APIs
builder.Services.AddControllersWithViews();

// Add debug logging in development
if (builder.Environment.IsDevelopment())
{
    builder.Logging.SetMinimumLevel(LogLevel.Information);
    builder.Logging.AddFilter("Microsoft.AspNetCore.Routing", LogLevel.Debug);
}

var app = builder.Build();

// Debug: List all registered routes in development
if (app.Environment.IsDevelopment())
{
    var logger = app.Services.GetRequiredService<ILogger<Program>>();
    
    // Log all registered endpoints after the app is built
    app.Lifetime.ApplicationStarted.Register(() =>
    {
        var endpointDataSource = app.Services.GetRequiredService<EndpointDataSource>();
        logger.LogInformation("=== Registered API Endpoints ===");
        foreach (var endpoint in endpointDataSource.Endpoints)
        {
            if (endpoint is RouteEndpoint routeEndpoint)
            {
                var methods = routeEndpoint.Metadata.GetMetadata<HttpMethodMetadata>()?.HttpMethods ?? new[] { "ANY" };
                logger.LogInformation("{Methods} {Pattern}", string.Join(",", methods), routeEndpoint.RoutePattern.RawText);
            }
        }
        logger.LogInformation("=== End of Endpoints ===");
    });
}

// Configure the HTTP request pipeline.
if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Home/Error");
    // The default HSTS value is 30 days. You may want to change this for production scenarios, see https://aka.ms/aspnetcore-hsts.
    app.UseHsts();
    app.UseHttpsRedirection(); // Only redirect to HTTPS in production
}

app.UseRouting();

app.UseAuthorization();

// Map API controllers (attribute routing)
app.MapControllers();

app.MapControllerRoute(
    name: "default",
    pattern: "{controller=Home}/{action=Index}/{id?}")
    .WithStaticAssets();


app.Run();
