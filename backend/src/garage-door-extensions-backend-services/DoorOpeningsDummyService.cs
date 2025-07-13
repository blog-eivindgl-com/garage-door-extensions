namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Services;

public class DoorOpeningsDummyService : IDoorOpeningsService
{
    private readonly IDateTimeService _dateTimeService;
    private readonly List<DateTimeOffset> _doorOpenings = new List<DateTimeOffset>();
    private readonly List<DateTimeOffset> _doorClosings = new List<DateTimeOffset>();

    public DoorOpeningsDummyService(IDateTimeService dateTimeService)
    {
        _dateTimeService = dateTimeService ?? throw new ArgumentNullException(nameof(dateTimeService));
    }

    public int GetDoorOpeningsToday()
    {
        // TODO: Implement actual logic to get today's door openings
        return 5; // Mock data
    }

    public int GetDoorOpeningsThisWeek()
    {
        // TODO: Implement actual logic to get this week's door openings
        return 25; // Mock data
    }

    public int GetDoorOpeningsThisMonth()
    {
        // TODO: Implement actual logic to get this month's door openings
        return 120; // Mock data
    }

    public void RegisterDoorOpening()
    {
        _doorOpenings.Add(_dateTimeService.Now);
    }

    public void RegisterDoorClosing()
    {
        _doorClosings.Add(_dateTimeService.Now);
    }

    public int GetDoorOpenDurationInSeconds()
    {
        if (_doorOpenings.Count == 0 || _doorClosings.Count == 0)
        {
            return 0; // No openings or closings registered
        }

        // Calculate total open duration
        var doorLastOpened = _doorOpenings.Max();
        var doorLastClosed = _doorClosings.Max();

        if (doorLastClosed < doorLastOpened)
        {
            return (int)Math.Floor(_dateTimeService.Now.Subtract(doorLastOpened).TotalSeconds);
        }

        return 0;
    }

    public long GetLastDoorOpened()
    {
        if (_doorOpenings.Count == 0)
        {
            return 0; // No openings registered
        }

        return _doorOpenings.Max().ToUnixTimeSeconds(); // Return the last door opened timestamp in seconds
    }

    public long GetLastDoorClosed()
    {
        if (_doorClosings.Count == 0)
        {
            return 0; // No closings registered
        }

        return _doorClosings.Max().ToUnixTimeSeconds(); // Return the last door closed timestamp in seconds
    }
}
