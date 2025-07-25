namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public class RfidUserViewModel
{
    public Guid UserId { get; set; }
    public string Name { get; set; }
    public string? Apartment { get; set; }
    public List<ValidDoorViewModel> ValidDoors { get; set; } = new List<ValidDoorViewModel>();
}