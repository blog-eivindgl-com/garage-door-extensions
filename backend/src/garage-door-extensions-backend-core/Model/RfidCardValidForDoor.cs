namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public class RfidCardValidForDoor
{
    public int Id { get; set; }
    public required string Rfid { get; set; }
    public required string DoorId { get; set; }
    public DateTime ValidFrom { get; set; }
    public DateTime? ValidTo { get; set; }
}