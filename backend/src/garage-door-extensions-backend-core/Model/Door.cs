namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public class Door
{
    public required string DoorId { get; init; }
    public required string Location { get; init; }
    public override string ToString() => $"{Location} ({DoorId})";
}
