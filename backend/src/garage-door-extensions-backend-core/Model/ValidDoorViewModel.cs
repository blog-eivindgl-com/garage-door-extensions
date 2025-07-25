namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public class ValidDoorViewModel
{
    public string DoorId { get; set; }
    public string Location { get; set; }
    public string Rfid { get; set; }
    public Guid BelongingUserId { get; set; }
    public string BelongingUserName { get; set; }
    public string? BelongingApartment { get; set; }
    public DateTime ValidFrom { get; set; }
    public DateTime? ValidTo { get; set; }
}