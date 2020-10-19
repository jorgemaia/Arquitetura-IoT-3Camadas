using System;
using System.Threading.Tasks;
using Newtonsoft.Json;

using Microsoft.Extensions.Logging;

using Microsoft.Azure.Devices;

namespace CrazyTechLabs.LoraIotHubBridge{

    static public class BridgeService
    {
        public static async Task Bridge(dynamic httpPayload, ILogger log)
        {
            log.LogInformation($"C# HTTP trigger function processed a request: {httpPayload.type}");
            if (httpPayload.type == "uplink")
            {
                // processes the message received
                string deviceId = httpPayload.meta.device_addr;
                dynamic loraPayload = httpPayload["params"];
                string base64Payload = loraPayload.payload;
                byte[] bytePayload = Convert.FromBase64String(base64Payload);
                string jsonPayload = System.Text.Encoding.UTF8.GetString(bytePayload);
                log.LogInformation(jsonPayload);
                dynamic payload = JsonConvert.DeserializeObject(jsonPayload);
                log.LogInformation($"Local: {payload.Local}\tTemp: {payload.T}\tUmi: {payload.U}\tLux: {payload.L}\n");

                // Sends the message to the IoT Hub
                Device device = await IoTHubService.GetDevice(deviceId);
                IoTHubService.SendMessage(device, payload);
            }
        }
    }
}