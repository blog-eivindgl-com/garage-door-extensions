public class AlertOptions
{
    public int GarageDoorOpenTimeoutSeconds { get; set; } = 120; // Default to 2 minutes
    public string AlertMessage { get; set; } = "Garage door has been open for {duration} seconds!";  // Use {duration} as placeholder for duration in seconds
}