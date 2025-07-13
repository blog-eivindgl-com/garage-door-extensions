using garage_door_extensions_backend.Components;
using garage_door_extensions_backend.Components.Pages;
using Microsoft.Extensions.Options;
using GarageDoorExtensionsBackend.Services;
using Microsoft.AspNetCore.Routing;

var builder = WebApplication.CreateBuilder(args);

// Enable detailed logging for development
if (builder.Environment.IsDevelopment())
{
    builder.Logging.SetMinimumLevel(LogLevel.Information);
    builder.Logging.AddFilter("Microsoft.AspNetCore.Routing", LogLevel.Debug);
    builder.Logging.AddFilter("Microsoft.AspNetCore.Mvc", LogLevel.Debug);
}

// Add services to the container.
builder.Services.AddHttpClient("DoorOpenings", options => 
{
    options.BaseAddress = new Uri(builder.Configuration.GetValue<string>("BaseUri"));
});
builder.Services.AddSingleton<IDoorOpeningsService, DoorOpeningsService>();

builder.Services.AddRazorComponents()
    .AddInteractiveServerComponents();
builder.Services.AddControllers();

var app = builder.Build();

// Debug: List all registered routes in development
if (app.Environment.IsDevelopment())
{
    var logger = app.Services.GetRequiredService<ILogger<Program>>();
    
    // Log all registered endpoints after the app is built
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
}

// Configure the HTTP request pipeline.
if (!app.Environment.IsDevelopment())
{
    app.UseExceptionHandler("/Error", createScopeForErrors: true);
    // The default HSTS value is 30 days. You may want to change this for production scenarios, see https://aka.ms/aspnetcore-hsts.
    app.UseHsts();
}

app.UseHttpsRedirection();

app.UseRouting();

app.UseAntiforgery();

// Map API controllers FIRST, before static files
app.MapControllers();

app.MapStaticAssets();
app.MapRazorComponents<App>()
    .AddInteractiveServerRenderMode();

app.Run();
