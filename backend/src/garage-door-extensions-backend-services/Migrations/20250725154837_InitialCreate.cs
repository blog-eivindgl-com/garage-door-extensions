using System;
using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Services.Migrations
{
    /// <inheritdoc />
    public partial class InitialCreate : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.CreateTable(
                name: "Apartments",
                columns: table => new
                {
                    Name = table.Column<string>(type: "TEXT", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Apartments", x => x.Name);
                });

            migrationBuilder.CreateTable(
                name: "Doors",
                columns: table => new
                {
                    DoorId = table.Column<string>(type: "TEXT", nullable: false),
                    Location = table.Column<string>(type: "TEXT", nullable: false)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_Doors", x => x.DoorId);
                });

            migrationBuilder.CreateTable(
                name: "RfidCards",
                columns: table => new
                {
                    Rfid = table.Column<string>(type: "TEXT", nullable: false),
                    BelongingUserId = table.Column<Guid>(type: "TEXT", nullable: false),
                    BelongingApartment = table.Column<string>(type: "TEXT", nullable: true)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_RfidCards", x => x.Rfid);
                });

            migrationBuilder.CreateTable(
                name: "RfidUsers",
                columns: table => new
                {
                    UserId = table.Column<Guid>(type: "TEXT", nullable: false),
                    Name = table.Column<string>(type: "TEXT", nullable: false),
                    Apartment = table.Column<string>(type: "TEXT", nullable: true)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_RfidUsers", x => x.UserId);
                });

            migrationBuilder.CreateTable(
                name: "RfidCardValidForDoors",
                columns: table => new
                {
                    Id = table.Column<int>(type: "INTEGER", nullable: false)
                        .Annotation("Sqlite:Autoincrement", true),
                    Rfid = table.Column<string>(type: "TEXT", nullable: false),
                    DoorId = table.Column<string>(type: "TEXT", nullable: false),
                    ValidFrom = table.Column<DateTime>(type: "TEXT", nullable: false),
                    ValidTo = table.Column<DateTime>(type: "TEXT", nullable: true)
                },
                constraints: table =>
                {
                    table.PrimaryKey("PK_RfidCardValidForDoors", x => x.Id);
                    table.ForeignKey(
                        name: "FK_RfidCardValidForDoors_Doors_DoorId",
                        column: x => x.DoorId,
                        principalTable: "Doors",
                        principalColumn: "DoorId",
                        onDelete: ReferentialAction.Cascade);
                    table.ForeignKey(
                        name: "FK_RfidCardValidForDoors_RfidCards_Rfid",
                        column: x => x.Rfid,
                        principalTable: "RfidCards",
                        principalColumn: "Rfid",
                        onDelete: ReferentialAction.Cascade);
                });

            migrationBuilder.CreateIndex(
                name: "IX_RfidCardValidForDoors_DoorId",
                table: "RfidCardValidForDoors",
                column: "DoorId");

            migrationBuilder.CreateIndex(
                name: "IX_RfidCardValidForDoors_Rfid",
                table: "RfidCardValidForDoors",
                column: "Rfid");
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropTable(
                name: "Apartments");

            migrationBuilder.DropTable(
                name: "RfidCardValidForDoors");

            migrationBuilder.DropTable(
                name: "RfidUsers");

            migrationBuilder.DropTable(
                name: "Doors");

            migrationBuilder.DropTable(
                name: "RfidCards");
        }
    }
}
