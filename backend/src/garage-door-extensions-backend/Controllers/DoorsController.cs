using BlogEivindGLCom.GarageDoorExtensionsBackend.Model;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

[ApiController]
[Route("api/[controller]")]
public class DoorsController : ControllerBase
{
    private readonly GarageDoorExtensionsDbContext _dbContext;

    public DoorsController(GarageDoorExtensionsDbContext dbContext)
    {
        _dbContext = dbContext ?? throw new ArgumentNullException(nameof(dbContext));
    }

    [HttpGet]
    public IActionResult GetDoors()
    {
        var doors = _dbContext.Doors.ToList();
        return Ok(doors);
    }

    [HttpGet("{doorId}")]
    public async Task<ActionResult<DoorViewModel>> GetDoor(string doorId)
    {
        var doorIdSafe = Uri.UnescapeDataString(doorId); // Handle URL encoding
        if (string.IsNullOrWhiteSpace(doorIdSafe))
        {
            return BadRequest("Door ID cannot be empty.");
        }

        var door = await _dbContext.Doors.FindAsync(doorIdSafe);
        if (door == null)
        {
            return NotFound();
        }

        // Map Door to DoorViewModel
        var validRfidCards = await (from validDoor in _dbContext.RfidCardValidForDoors
                         join rfidCard in _dbContext.RfidCards on validDoor.Rfid equals rfidCard.Rfid
                         join rfidUser in _dbContext.RfidUsers on rfidCard.BelongingUserId equals rfidUser.UserId
                         where validDoor.DoorId == doorIdSafe
                         select new ValidRfidCardViewModel
                         {
                            Rfid = rfidCard.Rfid,
                            BelongingUserId = rfidCard.BelongingUserId,
                            BelongingUserName = rfidUser.Name,
                            BelongingApartment = rfidCard.BelongingApartment,
                            ValidFrom = validDoor.ValidFrom,
                            ValidTo = validDoor.ValidTo
                         }).ToListAsync();

        var doorViewModel = new DoorViewModel
        {
            DoorId = door.DoorId,
            Location = door.Location,
            ValidRfidCards = validRfidCards
        };
        return Ok(doorViewModel);
    }

    [HttpPost]
    public async Task<IActionResult> AddDoor([FromBody] Door door)
    {
        if (door == null || string.IsNullOrWhiteSpace(door.DoorId) || string.IsNullOrWhiteSpace(door.Location))
        {
            return BadRequest("Invalid door data.");
        }

        _dbContext.Doors.Add(door);
        await _dbContext.SaveChangesAsync();
        return CreatedAtAction(nameof(GetDoor), new { DoorId = door.DoorId }, door);
    }

    [HttpDelete("{doorId}")]
    public async Task<IActionResult> DeleteDoor(string doorId)
    {
        var doorIdSafe = Uri.UnescapeDataString(doorId); // Handle URL encoding
        if (string.IsNullOrWhiteSpace(doorIdSafe))
        {
            return BadRequest("Door ID cannot be empty.");
        }

        var door = await _dbContext.Doors.FindAsync(doorIdSafe);
        if (door == null)
        {
            return NotFound();
        }

        _dbContext.Doors.Remove(door);
        await _dbContext.SaveChangesAsync();
        return NoContent();
    }
}