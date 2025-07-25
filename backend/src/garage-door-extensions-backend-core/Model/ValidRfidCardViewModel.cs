namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public class ValidRfidCardViewModel
{
    public string Rfid { get; set; }
    public Guid BelongingUserId { get; set; }
    public string BelongingUserName { get; set; }
    public string? BelongingApartment { get; set; }
    public DateTime ValidFrom { get; set; }
    public DateTime? ValidTo { get; set; }
}