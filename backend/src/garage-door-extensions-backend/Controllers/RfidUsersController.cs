using BlogEivindGLCom.GarageDoorExtensionsBackend.Model;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

[ApiController]
[Route("api/[controller]")]
public class RfidUsersController : ControllerBase
{
    private readonly GarageDoorExtensionsDbContext _dbContext;

    public RfidUsersController(GarageDoorExtensionsDbContext dbContext)
    {
        _dbContext = dbContext ?? throw new ArgumentNullException(nameof(dbContext));
    }

    [HttpGet]
    public IActionResult GetRfidUsers()
    {
        var rfidUsers = _dbContext.RfidUsers.ToList();
        return Ok(rfidUsers);
    }

    [HttpGet("{userId}")]
    public async Task<ActionResult<RfidUserViewModel>> GetRfidUser(string userId)
    {
        var userIdSafe = Uri.UnescapeDataString(userId); // Handle URL encoding
        if (string.IsNullOrWhiteSpace(userIdSafe))
        {
            return BadRequest("User ID cannot be empty.");
        }
        
        // Ensure userId is a valid Guid
        if (!Guid.TryParse(userIdSafe, out var parsedUserId))
        {
            return BadRequest("Invalid User ID format.");
        }

        // Assuming RfidUser has a property UserId that matches the userId parameter
        var rfidUser = await _dbContext.RfidUsers.FindAsync(parsedUserId);
        if (rfidUser == null)
        {
            return NotFound();
        }

        // Map RfidUser to RfidUserViewModel
        var validDoors = await (from validDoor in _dbContext.RfidCardValidForDoors
                         join door in _dbContext.Doors on validDoor.DoorId equals door.DoorId
                         join rfidCard in _dbContext.RfidCards on validDoor.Rfid equals rfidCard.Rfid
                         where rfidCard.BelongingUserId == rfidUser.UserId
                         select new ValidDoorViewModel
                         {
                            DoorId = door.DoorId,
                            Location = door.Location,
                            Rfid = rfidCard.Rfid,
                            BelongingUserId = rfidCard.BelongingUserId,
                            BelongingUserName = rfidUser.Name,
                            BelongingApartment = rfidCard.BelongingApartment,
                            ValidFrom = validDoor.ValidFrom,
                            ValidTo = validDoor.ValidTo
                         }).ToListAsync();
        var viewModel = new RfidUserViewModel
        {
            UserId = rfidUser.UserId,
            Name = rfidUser.Name,
            Apartment = rfidUser.Apartment,
            ValidDoors = validDoors
        };

        return Ok(viewModel);
    }

    [HttpPost]
    public async Task<IActionResult> AddRfidUser([FromBody] RfidUser rfidUser)
    {
        if (rfidUser == null || string.IsNullOrWhiteSpace(rfidUser.Name))
        {
            return BadRequest("Invalid RFID user data.");
        }

        if (rfidUser.Apartment != null)
        {
            if (_dbContext.Apartments.Find(rfidUser.Apartment) == null)
            {
                throw new ArgumentException("Apartment does not exist.");
            }
        }

        _dbContext.RfidUsers.Add(rfidUser);
        await _dbContext.SaveChangesAsync();
        return CreatedAtAction(nameof(GetRfidUser), new { userId = (rfidUser.UserId == Guid.Empty ? Guid.NewGuid() : rfidUser.UserId), name = rfidUser.Name, apartment = rfidUser.Apartment }, rfidUser);
    }

    [HttpDelete("{userId}")]
    public async Task<IActionResult> DeleteRfidUser(string userId)
    {
        var userIdSafe = Uri.UnescapeDataString(userId); // Handle URL encoding
        if (string.IsNullOrWhiteSpace(userIdSafe))
        {
            return BadRequest("User ID cannot be empty.");
        }
        
        // Ensure userId is a valid Guid
        if (!Guid.TryParse(userIdSafe, out var parsedUserId))
        {
            return BadRequest("Invalid User ID format.");
        }

        var rfidUser = await _dbContext.RfidUsers.FindAsync(parsedUserId);
        if (rfidUser == null)
        {
            return NotFound();
        }

        _dbContext.RfidUsers.Remove(rfidUser);
        await _dbContext.SaveChangesAsync();
        return NoContent();
    }
}