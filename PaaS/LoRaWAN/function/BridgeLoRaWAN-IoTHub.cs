using System.IO;
using System.Threading.Tasks;
using Newtonsoft.Json;

using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Http;
using Microsoft.Extensions.Logging;

using Microsoft.Azure.WebJobs;
using Microsoft.Azure.WebJobs.Extensions.Http;

namespace CrazyTechLabs.LoraIotHubBridge
{
    public static class tctec_lorawan
    {
        [FunctionName("tctec-lorawan")]
        public static async Task<IActionResult> Run(
            [HttpTrigger(AuthorizationLevel.Function, "get", "post", Route = null)] HttpRequest req,
            ILogger log)
        {
            string requestBody = await new StreamReader(req.Body).ReadToEndAsync();
            dynamic httpPayload = JsonConvert.DeserializeObject(requestBody);
            // log.LogInformation($"request body: {requestBody}");
            string responseMessage = "Received message!";
            await BridgeService.Bridge(httpPayload, log);
            return new OkObjectResult(responseMessage);
        }
    }
}