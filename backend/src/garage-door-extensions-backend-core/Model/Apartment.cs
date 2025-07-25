namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public class Apartment
{
    public required string Name { get; init; }

    public override string ToString() => Name;
}
