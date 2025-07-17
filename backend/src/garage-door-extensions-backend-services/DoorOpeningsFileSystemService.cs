using System.Globalization;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;

namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Services;

public class DoorOpeningsFileSystemService : IDoorOpeningsService
{
    private readonly ILogger<DoorOpeningsFileSystemService> _logger;
    private readonly IDateTimeService _dateTimeService;
    private string _doorOpenedBasePath;
    private string _doorClosedBasePath;

    public DoorOpeningsFileSystemService(IOptions<DoorOpeningsFileSystemOptions> options, ILogger<DoorOpeningsFileSystemService> logger, IDateTimeService dateTimeService)
    {
        if (options == null || options.Value == null)
        {
            throw new ArgumentNullException(nameof(options), "DoorOpeningsFileSystemOptions cannot be null");
        }
        _logger = logger ?? throw new ArgumentNullException(nameof(logger));
        _dateTimeService = dateTimeService ?? throw new ArgumentNullException(nameof(dateTimeService));

        var basePath = options.Value.BasePath;
        if (string.IsNullOrWhiteSpace(basePath))
        {
            basePath = Path.Combine(AppContext.BaseDirectory, "data");
        }
        _doorOpenedBasePath = Path.Combine(basePath, "door-opened");
        _doorClosedBasePath = Path.Combine(basePath, "door-closed"); 
    }

    public int GetDoorOpeningsToday()
    {
        string todayDir = GetOrCreateDirectoryTreeForDate(DoorState.Opened, _dateTimeService.Now.Date).FullName;
        return CountFilesInDirectory(todayDir);
    }

    public int GetDoorOpeningsThisWeek()
    {
        int doorOpeningsCount = 0;
        int dayOfWeek = (int)_dateTimeService.Now.DayOfWeek;
        int firstDayOfWeek = (int)CultureInfo.CurrentCulture.DateTimeFormat.FirstDayOfWeek;

        if (dayOfWeek == firstDayOfWeek)
        {
            var dir = GetOrCreateDirectoryTreeForDate(DoorState.Opened, _dateTimeService.Now.Date);
            doorOpeningsCount += CountFilesInDirectory(dir.FullName);
        }
        else
        {
            // Adjust to the first day of the week
            dayOfWeek = (dayOfWeek - firstDayOfWeek + 7) % 7; // Normalize to 0-6 range

            var startDate = _dateTimeService.Now.Date.AddDays(-dayOfWeek); // Adjust to first day of the week

            for (int i = 0; i < 7; i++)
            {
                var date = startDate.AddDays(i);

                if (date <= _dateTimeService.Now.Date)
                {
                    var dir = GetOrCreateDirectoryTreeForDate(DoorState.Opened, date);
                    doorOpeningsCount += CountFilesInDirectory(dir.FullName);
                }
            }
        }

        return doorOpeningsCount;
    }

    public int GetDoorOpeningsThisMonth()
    {
        int doorOpeningsCount = 0;
        var today = _dateTimeService.Now.Date;
        var startOfMonth = new DateTime(today.Year, today.Month, 1);

        for (var date = startOfMonth; date <= today; date = date.AddDays(1))
        {
            var dir = GetOrCreateDirectoryTreeForDate(DoorState.Opened, date);
            doorOpeningsCount += CountFilesInDirectory(dir.FullName);
        }

        return doorOpeningsCount;
    }

    public string GetDisplayData()
    {
        return $"{{"
            + $"\"today\": {GetDoorOpeningsToday()}, "
            + $"\"thisWeek\": {GetDoorOpeningsThisWeek()}, "
            + $"\"thisMonth\": {GetDoorOpeningsThisMonth()}, "
            + $"\"lastOpened\": {GetLastDoorOpened()}, "
            + $"\"lastClosed\": {GetLastDoorClosed()}, "
            + $"\"openDurationInSeconds\": {GetDoorOpenDurationInSeconds()}"
            + "}";
    } 

    public void RegisterDoorOpening()
    {
        CommonCreateFile(DoorState.Opened);
    }

    public void RegisterDoorClosing()
    {
        CommonCreateFile(DoorState.Closed);
    }

    private int CountFilesInDirectory(string path)
    {
        if (!Directory.Exists(path))
        {
            _logger.LogWarning("Directory does not exist: {Path}", path);
            return 0;
        }

        try
        {
            var files = Directory.GetFiles(path, "*.txt", SearchOption.AllDirectories);
            _logger.LogInformation("Found {Count} files in {Path}", files.Length, path);
            return files.Length;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, "Error counting files in directory: {Path}", path);
        }

        return 0;
    }

    private void CommonCreateFile(DoorState state)
    {
        string stateDescription = state == DoorState.Opened ? "opening" : "closing";

        // Add an empty file with now timestamp
        DirectoryInfo dir;
        try
        {
            dir = GetOrCreateDirectoryTreeForDate(state, _dateTimeService.Now.Date);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to create directory for door {stateDescription}");
            throw;
        }

        var now = _dateTimeService.Now;
        var filePath = Path.Combine(dir.FullName, $"{now:HH-mm-ss}.txt");

        try
        {
            if (!File.Exists(filePath))
            {
                using (var stream = File.Create(filePath))
                {
                    // Create an empty file
                    _logger.LogInformation($"Registered door {stateDescription} at {now} in {filePath}");
                }
            }
        }
        catch (IOException ioEx)
        {
            _logger.LogError(ioEx, $"IO error while registering door {stateDescription} at {filePath}");
            throw;
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Unexpected error while registering door {stateDescription} at {filePath}");
            throw;
        }
    }

    private DirectoryInfo GetOrCreateDirectoryTreeForDate(DoorState state, DateTime date)
    {
        var path = state switch
        {
            DoorState.Opened => Path.Combine(_doorOpenedBasePath, date.ToString("yyyy"), date.ToString("MM"), date.ToString("dd")),
            DoorState.Closed => Path.Combine(_doorClosedBasePath, date.ToString("yyyy"), date.ToString("MM"), date.ToString("dd")),
            _ => throw new ArgumentOutOfRangeException(nameof(state), state, null)
        };

        return GetOrCreateDirectory(path);
    }

    private DirectoryInfo GetOrCreateDirectory(string path)
    {   
        // Check if directory already exists before creating
        bool directoryExisted = Directory.Exists(path);
        
        // Directory.CreateDirectory automatically creates all parent directories if they don't exist
        var directoryInfo = Directory.CreateDirectory(path);
        
        if (!directoryExisted)
        {
            _logger.LogInformation("Created directory: {Path}", path);
        }
        
        return directoryInfo;
    }

    private FileInfo? GetLastFile(DoorState state)
    {
        var todaysPath = GetOrCreateDirectoryTreeForDate(state, _dateTimeService.Now.Date).FullName;
        var files = Directory.GetFiles(todaysPath, "*.txt", SearchOption.TopDirectoryOnly);

        // If no files found, check previous days up to 10 days
        for (int i = 0; i < 10; i++)
        {
            if (files.Length > 0)
            {
                // Look at the last file in the current directory
                var lastFile = files.OrderDescending().FirstOrDefault();

                if (lastFile != null)
                {
                    return new FileInfo(lastFile);
                }
            }
            else
            {
                // If no files found, check the previous day
                var path = GetOrCreateDirectoryTreeForDate(state, _dateTimeService.Now.Date.AddDays(i * -1)).FullName;
                files = Directory.GetFiles(path, "*.txt", SearchOption.TopDirectoryOnly);
            }
        }

        return null; // No files found in the last 10 days
    }

    private DateTimeOffset? GetDateTimeFromFile(FileInfo? file)
    {
        if (file != null)
        {
            // Extract date from file path
            var date = new DateOnly(int.Parse(file.Directory.Parent.Parent.Name), int.Parse(file.Directory.Parent.Name), int.Parse(file.Directory.Name));
            // Parse the time from the file name
            var time = TimeOnly.ParseExact(Path.GetFileNameWithoutExtension(file.Name), "HH-mm-ss", CultureInfo.InvariantCulture);

            return new DateTimeOffset(date.ToDateTime(time));
        }

        return null;    
    } 

    public int GetDoorOpenDurationInSeconds()
    {
        var lastOpenedFile = GetLastFile(DoorState.Opened);
        var lastOpenedTime = GetDateTimeFromFile(lastOpenedFile);

        if (lastOpenedTime.HasValue)
        {
            var lastClosedFile = GetLastFile(DoorState.Closed);
            var lastClosedTime = GetDateTimeFromFile(lastClosedFile);

            if (lastClosedTime.HasValue && lastClosedTime.Value > lastOpenedTime.Value)
            {
                // Door has been opened and closed, calculate duration
                return (int)(lastClosedTime.Value - lastOpenedTime.Value).TotalSeconds;
            }

            // If no closing file found, calculate duration from last opened time to now
            return (int)(_dateTimeService.Now - lastOpenedTime.Value).TotalSeconds;
        }

        _logger.LogWarning("No door opening files found in the last 10 days.");

        return 0; // No openings found
    }

    public long GetLastDoorOpened()
    {
        var lastFile = GetLastFile(DoorState.Opened);
        var lastOpenedTime = GetDateTimeFromFile(lastFile);

        if (lastOpenedTime.HasValue)
        {
            return lastOpenedTime.Value.ToUnixTimeSeconds();
        }

        _logger.LogWarning("No door opening files found in the last 10 days.");

        return 0; // No openings found
    }

    public long GetLastDoorClosed()
    {
        var lastFile = GetLastFile(DoorState.Closed);
        var lastClosedTime = GetDateTimeFromFile(lastFile);

        if (lastClosedTime.HasValue)
        {
            return lastClosedTime.Value.ToUnixTimeSeconds();
        }

        _logger.LogWarning("No door closing files found in the last 10 days.");

        return 0; // No closings found
    }

    public string GetLastDoorState()
    {
        var lastOpenedFile = GetLastFile(DoorState.Opened);
        var lastClosedFile = GetLastFile(DoorState.Closed);
        DateTimeOffset? lastOpenedTime = GetDateTimeFromFile(lastOpenedFile);
        DateTimeOffset? lastClosedTime = GetDateTimeFromFile(lastClosedFile);

        if (lastOpenedTime == null && lastClosedTime == null)
        {
            return "unknown"; // No state available
        }

        if (lastOpenedTime != null && (lastClosedTime == null || lastOpenedTime > lastClosedTime))
        {
            return "opened";
        }

        if (lastClosedTime != null && (lastOpenedTime == null || lastClosedTime > lastOpenedTime))
        {
            return "closed";
        }

        return "unknown"; // If both are equal, we can't determine the state
    }

    private enum DoorState
    {
        Opened,
        Closed
    }
}
