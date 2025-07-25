using BlogEivindGLCom.GarageDoorExtensionsBackend.Model;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

[ApiController]
[Route("api/[controller]")]
public class RfidCardsController : ControllerBase
{
    private readonly GarageDoorExtensionsDbContext _dbContext;

    public RfidCardsController(GarageDoorExtensionsDbContext dbContext)
    {
        _dbContext = dbContext ?? throw new ArgumentNullException(nameof(dbContext));
    }

    [HttpGet]
    public IActionResult GetRfidCards()
    {
        var rfidCards = _dbContext.RfidCards.ToList();
        return Ok(rfidCards); // Return all RFID cards
    }

    [HttpGet("{cardId}")]
    public async Task<ActionResult<RfidCardViewModel>> GetRfidCard(string cardId)
    {
        var cardIdSafe = Uri.UnescapeDataString(cardId); // Handle URL encoding
        if (string.IsNullOrWhiteSpace(cardIdSafe))
        {
            return BadRequest("Card ID cannot be empty.");
        }

        var rfidCard = await _dbContext.RfidCards.FindAsync(cardIdSafe);
        if (rfidCard == null)
        {
            return NotFound();
        }

        // Map to view model
        var validForDoors = await (from validRfidCard in _dbContext.RfidCardValidForDoors
                                    join c in _dbContext.RfidCards on validRfidCard.Rfid equals c.Rfid
                                    join u in _dbContext.RfidUsers on c.BelongingUserId equals u.UserId
                                    join door in _dbContext.Doors on validRfidCard.DoorId equals door.DoorId
                                    where validRfidCard.Rfid == c.Rfid
                                    select new ValidDoorViewModel
                                    {
                                        DoorId = validRfidCard.DoorId,
                                        Location = door.Location,
                                        Rfid = validRfidCard.Rfid,
                                        BelongingUserId = c.BelongingUserId,
                                        BelongingUserName = u.Name,
                                        BelongingApartment = c.BelongingApartment,
                                        ValidFrom = validRfidCard.ValidFrom,
                                        ValidTo = validRfidCard.ValidTo
                                    }).ToListAsync();
        var rfidUser = await _dbContext.RfidUsers.FindAsync(rfidCard.BelongingUserId);
        var viewModel = new RfidCardViewModel
        {
            Rfid = rfidCard.Rfid,
            BelongingUserId = rfidCard.BelongingUserId,
            BelongingUserName = rfidUser?.Name,
            BelongingApartment = rfidCard.BelongingApartment,
            ValidForDoors = validForDoors
        };

        return Ok(viewModel);
    }

    [HttpGet("free")]
    public async Task<ActionResult<IEnumerable<RfidCard>>> GetFreeRfidCards()
    {
        var freeRfidCards = _dbContext.RfidCards
            .Where(card => card.BelongingUserId == Guid.Empty && string.IsNullOrWhiteSpace(card.BelongingApartment))
            .ToList();

        return Ok(freeRfidCards);
    }
    
    [HttpPost]
    public async Task<IActionResult> AddRfidCard([FromBody] RfidCard rfidCard)
    {
        if (rfidCard == null || string.IsNullOrWhiteSpace(rfidCard.Rfid))
        {
            return BadRequest("Invalid RFID card data.");
        }

        var existingCard = await _dbContext.RfidCards.FindAsync(rfidCard.Rfid);
        if (existingCard != null)
        {
            return Conflict("RFID card already exists.");
        }

        if (rfidCard.BelongingUserId != Guid.Empty)
        {
            // If the card is assigned to a user, we need to check if the user exists
            var user = await _dbContext.RfidUsers.FindAsync(rfidCard.BelongingUserId);
            if (user == null)
            {
                return BadRequest("Invalid user ID.");
            }
        }

        if (rfidCard.BelongingApartment != null)
        {
            // If the card is assigned to an apartment, we need to check if the apartment exists
            var apartment = await _dbContext.Apartments.FindAsync(rfidCard.BelongingApartment);
            if (apartment == null)
            {
                return BadRequest("Invalid apartment ID.");
            }
        }

        _dbContext.RfidCards.Add(rfidCard);
        await _dbContext.SaveChangesAsync();

        return CreatedAtAction(nameof(GetRfidCard), new { cardId = rfidCard.Rfid }, rfidCard);
    }

    [HttpPut("{rfid}")]
    public async Task<IActionResult> UpdateRfidCard(string rfid, [FromBody] RfidCard updatedCard)
    {
        if (string.IsNullOrWhiteSpace(rfid) || updatedCard == null)
        {
           return BadRequest("Invalid RFID card data.");
        }

        var rfidSafe = Uri.UnescapeDataString(rfid); // Handle URL encoding
        var existingCard = await _dbContext.RfidCards.FindAsync(rfidSafe);
        if (existingCard == null)
        {
           return NotFound();
        }


        if (updatedCard.BelongingUserId != Guid.Empty)
        {
            // If the card is assigned to a user, we need to check if the user exists
            var user = await _dbContext.RfidUsers.FindAsync(updatedCard.BelongingUserId);
            if (user == null)
            {
                return BadRequest("Invalid user ID.");
            }
        }

        if (updatedCard.BelongingApartment != null)
        {
            // If the card is assigned to an apartment, we need to check if the apartment exists
            var apartment = await _dbContext.Apartments.FindAsync(updatedCard.BelongingApartment);
            if (apartment == null)
            {
                return BadRequest("Invalid apartment ID.");
            }
        }

        existingCard.BelongingUserId = updatedCard.BelongingUserId;
        existingCard.BelongingApartment = updatedCard.BelongingApartment;
        await _dbContext.SaveChangesAsync();

        return Ok(existingCard);
    }

    [HttpDelete("{cardId}")]
    public async Task<IActionResult> DeleteRfidCard(string cardId)
    {
        var cardIdSafe = Uri.UnescapeDataString(cardId); // Handle URL encoding
        if (string.IsNullOrWhiteSpace(cardIdSafe))
        {
            return BadRequest("Card ID cannot be empty.");
        }

        var rfidCard = await _dbContext.RfidCards.FindAsync(cardIdSafe);
        if (rfidCard == null)
        {
            return NotFound();
        }

        _dbContext.RfidCards.Remove(rfidCard);
        await _dbContext.SaveChangesAsync();

        return NoContent();
    }

    [HttpPost("valid-for-door")]
    public async Task<IActionResult> MakeRfidCardValidForDoor([FromBody] RfidCardValidForDoor request)
    {
        if (request == null || string.IsNullOrWhiteSpace(request.Rfid) || string.IsNullOrWhiteSpace(request.DoorId))
        {
            return BadRequest("Invalid request.");
        }

        if (request.ValidFrom == default)
        {
            request.ValidFrom = DateTime.UtcNow; // Default to now if not provided
        }

        if (request.ValidTo.HasValue && request.ValidTo < request.ValidFrom)
        {
            return BadRequest("ValidTo cannot be earlier than ValidFrom.");
        }

        if (request.ValidTo.HasValue && request.ValidTo < DateTime.UtcNow)
        {
            return BadRequest("ValidTo cannot be in the past.");
        }

        var rfidCard = await _dbContext.RfidCards.FindAsync(request.Rfid);
        if (rfidCard == null)
        {
            return BadRequest("RFID card not found.");
        }

        var door = await _dbContext.Doors.FindAsync(request.DoorId);
        if (door == null)
        {
            return BadRequest("Door not found.");
        }

        // Check if the card is already valid for this door
        var existingEntries = _dbContext.RfidCardValidForDoors
            .Where(e => e.Rfid == request.Rfid && e.DoorId == request.DoorId);
        if (existingEntries.Any())
        {
            _dbContext.RfidCardValidForDoors.RemoveRange(existingEntries);
        }
        var newEntry = new RfidCardValidForDoor
        {
            Rfid = request.Rfid,
            DoorId = request.DoorId,
            ValidFrom = request.ValidFrom,
            ValidTo = request.ValidTo
        };
        _dbContext.RfidCardValidForDoors.Add(newEntry);
        await _dbContext.SaveChangesAsync();

        return Ok(newEntry);
    }
} 