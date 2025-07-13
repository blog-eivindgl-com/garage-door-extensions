namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Services;

using System.Threading.Tasks;

public interface IDoorOpeningsService
{
    int GetDoorOpenDurationInSeconds();
    int GetDoorOpeningsToday();
    int GetDoorOpeningsThisWeek();
    int GetDoorOpeningsThisMonth();
    long GetLastDoorOpened();
    long GetLastDoorClosed();
    void RegisterDoorOpening();
    void RegisterDoorClosing();
}