using BlogEivindGLCom.GarageDoorExtensionsBackend.Model;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.Options;
using MQTTnet;

[ApiController]
[Route("api/[controller]")]
public class DoorsController : ControllerBase
{
    private const string QueryValidRfidCardsTopic = "garageDoor/queryValidRfidCards";
    private readonly GarageDoorExtensionsDbContext _dbContext;
    private readonly IMqttClient _mqttClient;
    private readonly MQTTnet.MqttClientOptions _mqttClientOptions;
    private readonly ILogger<DoorsController> _logger;

    public DoorsController(GarageDoorExtensionsDbContext dbContext, IOptions<MqttClientOptions> mqttClientOptions, ILogger<DoorsController> logger)
    {
        _dbContext = dbContext ?? throw new ArgumentNullException(nameof(dbContext));
        _logger = logger ?? throw new ArgumentNullException(nameof(logger));

        var mqttFactory = new MqttClientFactory();
        _mqttClient = mqttFactory.CreateMqttClient();
        _mqttClientOptions = new MqttClientOptionsBuilder()
            .WithTcpServer(mqttClientOptions.Value.BrokerAddress, port: mqttClientOptions.Value.Port)
            .WithCredentials(mqttClientOptions.Value.Username, mqttClientOptions.Value.Password)
            .Build();
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

   [HttpPost("publishValidRfidCards")]
   public async Task<IActionResult> PublishValidRfidCards()
   {
        _logger.LogInformation("MQTT client is not connected, attempting to connect...");

        // Publish a message to have all doors query their valid RFID cards
        using (var cts = new CancellationTokenSource(TimeSpan.FromSeconds(5)))
        {
            var result = await _mqttClient.ConnectAsync(_mqttClientOptions, cts.Token);

            if (result.ResultCode != MQTTnet.MqttClientConnectResultCode.Success)
            {
                _logger.LogError($"Failed to connect to MQTT broker: {result.ReasonString}");
                return StatusCode(500, "Failed to connect to MQTT broker."); // Exit if connection fails
            }

            _logger.LogInformation("The MQTT client is connected.");

            await _mqttClient.PublishAsync(new MqttApplicationMessageBuilder()
               .WithTopic(QueryValidRfidCardsTopic)
               .Build(), cts.Token);
       
            await _mqttClient.DisconnectAsync();
        } 

        _logger.LogInformation("Published MQTT message for all doors to query valid RFID cards.");

        return NoContent();
   }
}