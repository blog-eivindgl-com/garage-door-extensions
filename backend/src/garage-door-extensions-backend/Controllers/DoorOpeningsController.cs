namespace BlogEivindGLCom.GarageDoorExtensionsBackend.Controllers;
using BlogEivindGLCom.GarageDoorExtensionsBackend.Services;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Mvc;

[ApiController]
[Route("api/[controller]")]
public class DoorOpeningsController : ControllerBase
{
    private readonly IDoorOpeningsService _doorOpeningService;

    public DoorOpeningsController(IDoorOpeningsService doorOpeningsService)
    {
        _doorOpeningService = doorOpeningsService;
    }

    [HttpGet("today")]
    public IActionResult GetDoorOpeningsToday()
    {
        var openings = _doorOpeningService.GetDoorOpeningsToday();
        return Ok(openings);
    }

    [HttpGet("week")]
    public IActionResult GetDoorOpeningsThisWeek()
    {
        var openings = _doorOpeningService.GetDoorOpeningsThisWeek();
        return Ok(openings);
    }

    [HttpGet("month")]
    public IActionResult GetDoorOpeningsThisMonth()
    {
        var openings = _doorOpeningService.GetDoorOpeningsThisMonth();
        return Ok(openings);
    }

    [HttpGet("display")]
    public IActionResult GetDisplayData()
    {
        var displayData = _doorOpeningService.GetDisplayData();
        return Ok(displayData);
    }

    [HttpGet("openduration")]
    public IActionResult GetDoorOpenDuration()
    {
        var openduration = _doorOpeningService.GetDoorOpenDurationInSeconds();
        return Ok(openduration);
    } 

    [HttpGet("lastopened")]
    public IActionResult GetLastDoorOpened()
    {
        var lastDoorOpened = _doorOpeningService.GetLastDoorOpened();
        return Ok(lastDoorOpened);
    }

    [HttpGet("lastclosed")]
    public IActionResult GetLastDoorClosed()
    {
        var lastDoorClosed = _doorOpeningService.GetLastDoorClosed();
        return Ok(lastDoorClosed);
    } 

    [HttpPost("RegisterDoorOpening")]
    public IActionResult RegisterDoorOpening()
    {
        _doorOpeningService.RegisterDoorOpening();
        return NoContent();
    }

    [HttpPost("RegisterDoorClosing")]
    public IActionResult RegisterDoorClosing()
    {
        _doorOpeningService.RegisterDoorClosing();
        return NoContent();
    }
}