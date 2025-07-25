namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Model;

using System.Reflection.Emit;
using Microsoft.EntityFrameworkCore;

public class GarageDoorExtensionsDbContext : DbContext
{
    public GarageDoorExtensionsDbContext(DbContextOptions<GarageDoorExtensionsDbContext> options) : base(options) { }

    public DbSet<Apartment> Apartments { get; set; } = null!;
    public DbSet<Door> Doors { get; set; } = null!;
    public DbSet<RfidUser> RfidUsers { get; set; } = null!;
    public DbSet<RfidCard> RfidCards { get; set; } = null!;
    public DbSet<RfidCardValidForDoor> RfidCardValidForDoors { get; set; } = null!;

    protected override void OnModelCreating(ModelBuilder modelBuilder)
    {
        modelBuilder.Entity<Apartment>().HasKey(a => a.Name);
        modelBuilder.Entity<Door>().HasKey(d => d.DoorId);
        modelBuilder.Entity<RfidUser>().HasKey(u => u.UserId);
        modelBuilder.Entity<RfidCard>().HasKey(c => c.Rfid);
        modelBuilder.Entity<RfidCardValidForDoor>().HasKey(v => new { v.Id });
        modelBuilder.Entity<RfidCardValidForDoor>()
            .HasOne<Door>()
            .WithMany()
            .HasForeignKey(v => v.DoorId)
            .OnDelete(DeleteBehavior.Cascade);
        modelBuilder.Entity<RfidCardValidForDoor>()
            .HasOne<RfidCard>()
            .WithMany()
            .HasForeignKey(v => v.Rfid)
            .OnDelete(DeleteBehavior.Cascade);
    }
}