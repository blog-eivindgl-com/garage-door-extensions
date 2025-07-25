namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Services;

using BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public interface IRfidUserStorage
{
    Task StoreUserAsync(Guid userId, string name, string apartment);
    Task<IEnumerable<RfidUser>> GetAllUsersAsync();
    Task<RfidUser> GetUserAsync(Guid userId);
}
