namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public class RfidUser
{
    public Guid UserId { get; init; }
    public required string Name { get; init; }
    public string? Apartment { get; init; }

    public override string ToString() => $"{Name} ({UserId})";
}
