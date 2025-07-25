using BlogEivindGLCom.GarageDoorExtensionsBackend.Model;
using Microsoft.AspNetCore.Mvc;

[ApiController]
[Route("api/[controller]")]
public class ApartmentsController : ControllerBase
{
    private readonly GarageDoorExtensionsDbContext _dbContext;

    public ApartmentsController(GarageDoorExtensionsDbContext dbContext)
    {
        _dbContext = dbContext ?? throw new ArgumentNullException(nameof(dbContext));
    }

    [HttpGet]
    public IActionResult GetApartments()
    {
        var apartments = _dbContext.Apartments.ToList();
        return Ok(apartments);
    }

    [HttpGet("{name}")]
    public async Task<ActionResult<Apartment>> GetApartment(string name)
    {
        var apartment = await _dbContext.Apartments.FindAsync(name);
        if (apartment == null)
        {
            return NotFound();
        }
        return Ok(apartment);
    }

    [HttpPost]
    public async Task<IActionResult> AddApartment([FromBody] Apartment apartment)
    {
        if (apartment == null || string.IsNullOrWhiteSpace(apartment.Name))
        {
            return BadRequest("Invalid apartment data.");
        }

        _dbContext.Apartments.Add(apartment);
        await _dbContext.SaveChangesAsync();
        return CreatedAtAction(nameof(GetApartment), new { name = apartment.Name }, apartment);
    }

    [HttpDelete("{name}")]
    public async Task<IActionResult> DeleteApartment(string name)
    {
        var apartment = await _dbContext.Apartments.FindAsync(name);
        if (apartment == null)
        {
            return NotFound();
        }

        _dbContext.Apartments.Remove(apartment);
        await _dbContext.SaveChangesAsync();
        return NoContent();
    }
}