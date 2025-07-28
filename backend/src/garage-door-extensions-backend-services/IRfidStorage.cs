namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Services;

using BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public interface IRfidStorage
{
    void StoreRfid(string rfid);
    IOrderedEnumerable<InvalidRfid> GetAllInvalidRfids();
    IOrderedEnumerable<InvalidRfid> GetInvalidRfids(DateTime from, DateTime? to);
}
