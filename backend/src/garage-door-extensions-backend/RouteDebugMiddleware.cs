namespace garage_door_extensions_backend;

public class RouteDebugMiddleware
{
    private readonly RequestDelegate _next;
    private readonly ILogger<RouteDebugMiddleware> _logger;

    public RouteDebugMiddleware(RequestDelegate next, ILogger<RouteDebugMiddleware> logger)
    {
        _next = next;
        _logger = logger;
    }

    public async Task InvokeAsync(HttpContext context)
    {
        var endpoint = context.GetEndpoint();
        if (endpoint != null)
        {
            _logger.LogInformation("Matched endpoint: {DisplayName} for {Method} {Path}", 
                endpoint.DisplayName, 
                context.Request.Method, 
                context.Request.Path);
        }
        else
        {
            _logger.LogWarning("No endpoint matched for {Method} {Path}", 
                context.Request.Method, 
                context.Request.Path);
        }

        await _next(context);
    }
}
