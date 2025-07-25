namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Model;
public class RfidCard
{
    public required string Rfid { get; init; }
    public Guid BelongingUserId { get; set; }
    public string? BelongingApartment { get; set; }

    public override string ToString() => $"{Rfid}";
}
