namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public class RfidCardViewModel
{
    public string Rfid { get; set; }
    public Guid BelongingUserId { get; set; }
    public string BelongingUserName { get; set; }
    public string? BelongingApartment { get; set; }
    public List<ValidDoorViewModel> ValidForDoors { get; set; } = new List<ValidDoorViewModel>();
}