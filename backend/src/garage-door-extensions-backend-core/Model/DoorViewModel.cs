namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public class DoorViewModel
{
    public string DoorId { get; set; }
    public string Location { get; set; }
    public List<ValidRfidCardViewModel> ValidRfidCards { get; set; } = new List<ValidRfidCardViewModel>();
}