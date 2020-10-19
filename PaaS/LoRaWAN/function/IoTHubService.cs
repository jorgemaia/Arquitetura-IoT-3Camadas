using System;
using System.Threading.Tasks;
using Newtonsoft.Json;
using System.Dynamic;
using System.Collections.Generic;
using System.Linq;
using System.ComponentModel;
using System.Text;

using Microsoft.Azure.Devices;
using Microsoft.Azure.Devices.Client;
using Microsoft.Extensions.Logging;

namespace CrazyTechLabs.LoraIotHubBridge{

    public static class IoTHubService
    {
        static private RegistryManager RegistryMgr;
        
        // IoTHub Connection String with read permitions
        static private readonly string ConnectionString = "STRING CONEXÃO COM O HUB (USE A OWNER";
        static private readonly string HostName = "ddddddd.azure-devices.net";
        // Query to get all modules reported information from IoTHub
        static private readonly string Query =  "SELECT deviceId, status FROM devices";

        /// <sumary>
        /// Runs query to retrive all modules latest reported information
        /// returns: List of dynamic objects with all modules reported status
        /// </sumary>
        public static async Task<List<ExpandoObject>> RunQuery()
        {
            var ret = new List<ExpandoObject>();
            RegistryMgr = RegistryManager.CreateFromConnectionString(ConnectionString);
            var query = RegistryMgr.CreateQuery(Query, 100);
            var result = await query.GetNextAsJsonAsync();
            
            // foreach (dynamic item in result)
            // {
            //     dynamic dynamicResult = JsonConvert.DeserializeObject(item);
            //     DeviceName = dynamicResult.deviceId;
            //     ret.AddRange(GetModules(dynamicResult.modules));
            //     ret.AddRange(GetModules(dynamicResult.systemModules));
            // }
            return ret;
        }

        /// <sumary>
        /// Get the device with the given ID or creates it in the IoT Hub if it does not exists
        /// <param name="deviceId">The id to be used to create the device</param>
        /// <returns>The device with the given id.</returns>
        /// </sumary>
        public static async Task<Device> GetDevice(string deviceId)
        {
            RegistryMgr = RegistryManager.CreateFromConnectionString(ConnectionString);
            List<Device> devices = (await RegistryMgr.GetDevicesAsync(999)).ToList();
            Device device = null;
            devices.ForEach(d => {if(d.Id == deviceId) device = d;});
            if(device == null)
            {
                device = await RegistryMgr.AddDeviceAsync(new Device(deviceId));
            }
            return device;
        }

        /// <sumary>
        /// Creates a device in the IoT Hub with the given ID
        /// <param name="deviceId">The id to be used to create the device</param>
        /// <returns>A boolean indicating wheter the message was sent or not</returns>
        /// </sumary>
        public static void SendMessage(Device device, dynamic json)
        {
            // create device client
            DeviceAuthenticationWithRegistrySymmetricKey auth = new DeviceAuthenticationWithRegistrySymmetricKey(device.Id, device.Authentication.SymmetricKey.PrimaryKey);
            DeviceClient _deviceClient = DeviceClient.Create(HostName, auth, Microsoft.Azure.Devices.Client.TransportType.Amqp);
            
            // builds message
            var messageString = JsonConvert.SerializeObject(json);
            var message = new Microsoft.Azure.Devices.Client.Message(Encoding.ASCII.GetBytes(messageString));
            int temp;
            if(int.TryParse((json.T as string), out temp)) message.Properties.Add("temperatureAlert", (temp > 30) ? "true" : "false");

            // sends message
            _deviceClient.SendEventAsync(message).Wait();
        }
    }
}