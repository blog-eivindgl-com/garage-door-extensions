namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Model;
public class InvalidRfid
{
    public required string Rfid { get; init; }
    public DateTimeOffset UsedAt { get; init; } = DateTime.UtcNow;
}
