using BlogEivindGLCom.GarageDoorExtensionsBackend.Model;
using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.Design;

public class GarageDoorExtensionsDbContextFactory : IDesignTimeDbContextFactory<GarageDoorExtensionsDbContext>
{
    public GarageDoorExtensionsDbContext CreateDbContext(string[] args)
    {
        var connectionString = "Data Source=garage_door_extensions.db"; // Default connection string
        if (args.Length > 0)
        {
            connectionString = args[0]; // Use the first argument as the connection string
        }

        return CreateDbContext(connectionString);
    }

    public GarageDoorExtensionsDbContext CreateDbContext(string connectionString)
    {
        var optionsBuilder = new DbContextOptionsBuilder<GarageDoorExtensionsDbContext>();
        optionsBuilder.UseSqlite(connectionString);

        return new GarageDoorExtensionsDbContext(optionsBuilder.Options);
    }
}