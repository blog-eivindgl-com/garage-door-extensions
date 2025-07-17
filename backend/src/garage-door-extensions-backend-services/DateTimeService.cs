namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Services;

public class DateTimeService : IDateTimeService
{
    public DateTimeOffset Now => DateTimeOffset.Now;
}
