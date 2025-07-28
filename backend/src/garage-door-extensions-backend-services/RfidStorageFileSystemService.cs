using System.Globalization;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;
using BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Services;

public class RfidStorageFileSystemService : IRfidStorage
{
    private readonly ILogger<RfidStorageFileSystemService> _logger;
    private readonly IDateTimeService _dateTimeService;
    private string _invalidRfidsPath;

    public RfidStorageFileSystemService(IOptions<RfidStorageFileSystemOptions> options, ILogger<RfidStorageFileSystemService> logger, IDateTimeService dateTimeService)
    {
        if (options == null || options.Value == null)
        {
            throw new ArgumentNullException(nameof(options), "RfidStorageFileSystemOptions cannot be null");
        }
        _logger = logger ?? throw new ArgumentNullException(nameof(logger));
        _dateTimeService = dateTimeService ?? throw new ArgumentNullException(nameof(dateTimeService));

        var basePath = options.Value.BasePath;
        if (string.IsNullOrWhiteSpace(basePath))
        {
            basePath = Path.Combine(AppContext.BaseDirectory, "data");
        }
        _invalidRfidsPath = Path.Combine(basePath, "invalid-rfids");
    }

    public void StoreRfid(string rfid)
    {
        if (string.IsNullOrWhiteSpace(rfid))
        {
            throw new ArgumentException("RFID cannot be null or empty", nameof(rfid));
        }

        var invalidRfid = new InvalidRfid { Rfid = rfid, UsedAt = _dateTimeService.Now };
        CreateFile(invalidRfid);
    } 

    public IOrderedEnumerable<InvalidRfid> GetAllInvalidRfids()
    {
        // Going more than 30 days back would be too time-consuming
        _logger.LogInformation("Retrieving all invalid RFIDs from the last 30 days");
        // Get all invalid RFIDs from the last 30 days
        return GetInvalidRfids(_dateTimeService.Now.Date.AddDays(-30), _dateTimeService.Now.Date);
    }

    public IOrderedEnumerable<InvalidRfid> GetInvalidRfids(DateTime from, DateTime? to)
    {
        // Implementation for retrieving invalid RFIDs within a specific date range
        if (to == null)
        {
            to = _dateTimeService.Now.Date;
        }

        if (from > to)
        {
            throw new ArgumentException("From date cannot be later than To date");
        }

        if (from < _dateTimeService.Now.Date.AddYears(-1))
        {
            throw new ArgumentException("Cannot retrieve invalid RFIDs from more than 1 year ago");
        }

        if (to > _dateTimeService.Now.Date)
        {
            throw new ArgumentException("To date cannot be in the future");
        }

        var invalidRfids = new List<InvalidRfid>();

        while (from <= to)
        {
            var directory = GetOrCreateDirectoryTreeForDate(from.Date);
            var files = Directory.GetFiles(directory.FullName, "*.txt", SearchOption.TopDirectoryOnly);
            foreach (var file in files)
            {
                var rfid = File.ReadAllText(file).Trim();
                var fileDateTime = GetDateTimeFromFile(new FileInfo(file));

                if (fileDateTime == null)
                {
                    _logger.LogWarning("File {File} has an invalid timestamp, skipping", file);
                    continue; // Skip files with invalid timestamps
                }

                invalidRfids.Add(new InvalidRfid 
                { 
                    Rfid = rfid, 
                    UsedAt = fileDateTime.Value
                });
            }
            from = from.AddDays(1);
        }

        return invalidRfids.OrderByDescending(r => r.UsedAt);
    }

    private void CreateFile(InvalidRfid rfid)
    {
        // Add a file with now timestamp with the rfid as content
        DirectoryInfo dir;
        try
        {
            dir = GetOrCreateDirectoryTreeForDate(rfid.UsedAt.Date);
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to create directory for RFID {rfid.Rfid}");
            throw;
        }

        var filePath = Path.Combine(dir.FullName, $"{rfid.UsedAt:HH-mm-ss}.txt");

        try
        {
            using (var stream = File.Create(filePath))
            {
                using (var writer = new StreamWriter(stream))
                {
                    writer.WriteLine(rfid.Rfid);
                }
            }
        }
        catch (Exception ex)
        {
            _logger.LogError(ex, $"Failed to create file for RFID {rfid.Rfid}");
            throw;
        }
    }

    private DirectoryInfo GetOrCreateDirectoryTreeForDate(DateTime date)
    {
        var path = Path.Combine(_invalidRfidsPath, date.ToString("yyyy"), date.ToString("MM"), date.ToString("dd"));
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
}
