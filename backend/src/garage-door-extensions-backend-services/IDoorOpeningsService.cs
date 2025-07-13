namespace GarageDoorExtensionsBackend.Services;

using System.Threading.Tasks;

public interface IDoorOpeningsService
{
    Task<int> GetDoorOpeningsTodayAsync();
    Task<int> GetDoorOpeningsThisWeekAsync();
    Task<int> GetDoorOpeningsThisMonthAsync();
    Task RegisterDoorOpeningAsync();
    Task RegisterDoorClosingAsync();
}