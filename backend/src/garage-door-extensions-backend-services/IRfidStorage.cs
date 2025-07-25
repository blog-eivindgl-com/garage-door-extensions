namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Services;

using BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public interface IRfidStorage
{
    Task StoreRfidAsync(string rfid, string name, string apartment);
    Task MakeRfIdInvalidAsync(string rfid);
    Task MakeRfIdValidForDoorsAsync(string rfid, IEnumerable<string> doorIds);
    Task<IEnumerable<RfidCard>> GetAllRfidsAsync();
    Task<IEnumerable<RfidCard>> GetAllValidRfidsForDoorAsync(string doorId);
    Task<IEnumerable<RfidCard>> GetAllValidRfidsForApartmentAsync(string apartment);
    Task<IEnumerable<RfidCard>> GetAllValidRfidsForUserAsync(Guid userId);
    Task<RfidCard> GetRfidAsync();
}
