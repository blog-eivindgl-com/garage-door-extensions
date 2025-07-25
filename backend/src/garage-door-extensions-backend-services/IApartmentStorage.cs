namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

public interface IApartmentStorage
{
    Task<Apartment?> GetApartmentAsync(string name);
    Task<IEnumerable<Apartment>> GetAllApartmentsAsync();
    Task AddApartmentAsync(Apartment apartment);
    Task DeleteApartmentAsync(string name);
}