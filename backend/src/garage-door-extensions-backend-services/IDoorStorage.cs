namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Services;

using BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public interface IDoorStorage
{
    Task StoreDoorAsync(Door door);
    Task<IEnumerable<Door>> GetAllDoorsAsync();
    Task<Door> GetDoorAsync(string doorId);
    Task DeleteDoorAsync(string doorId);
}
