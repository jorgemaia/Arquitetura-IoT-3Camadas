using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Azure.Devices.Client;
using Microsoft.Azure.WebJobs;
using Microsoft.Azure.WebJobs.Extensions.EdgeHub;
using Microsoft.Azure.WebJobs.Host;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using System.Linq;
using System.Dynamic;

namespace Functions.Samples
{
    public static class ProcessamentoLocal
    {
        [FunctionName("ProcessamentoLocal")]
        public static async Task FilterMessageAndSendMessage(
                    [EdgeHubTrigger("input1")] Message messageReceived,
                    [EdgeHub(OutputName = "output1")] IAsyncCollector<Message> output,
                    ILogger logger)
        {
            byte[] messageBytes = messageReceived.GetBytes();
            var messageString = System.Text.Encoding.UTF8.GetString(messageBytes);
            logger.LogInformation($"Mensagem recebida: {messageString}");
            List<msgmodbus> MsgsInput = JsonConvert.DeserializeObject<List<msgmodbus>>(messageString);
            
  
            string[] l = MsgsInput.Select(b=> b.DisplayName).Distinct().ToArray();

            foreach(var i in l)
            {
                var lista = MsgsInput.Where(f=> f.DisplayName == i).OrderBy(e=> e.SourceTimestamp).ThenBy(g=> g.Address).ToArray();
                
                
                for(int a=0; a < lista.Count(); a=a+2) 
                {
                    
                    
                    byte[] primeiraparte = BitConverter.GetBytes(UInt16.Parse(lista[a].Value));
                    byte[] segundaparte = BitConverter.GetBytes(UInt16.Parse(lista[a+1].Value));

                    var palavra32 = new byte[4] {primeiraparte[0], primeiraparte[1], segundaparte[0], segundaparte[1]}; 
                    var valorpreliminar = (double) BitConverter.ToInt32(palavra32,0);
                    
                    dynamic payload = new ExpandoObject();
                    payload.Medida = lista[a].DisplayName;
                    payload.Hardware = lista[a].HwId;
                    payload.DataHora = lista[a].SourceTimestamp; 

                    switch(lista[a].DisplayName)
                    {
                        case "Voltagem Fase 1":
                            payload.ValorMedida = ((double) valorpreliminar / 10.0 );
                        break;

                        case "Amperagem Fase 1 - (Atual)":
                            payload.ValorMedida = ((double) valorpreliminar / 1000.0 );
                        break;
                        default: 
                        payload.ValorMedida = valorpreliminar;
                        break;
                    }
                    var mensagemtexto = JsonConvert.SerializeObject(payload);
                    Message msg = new Message( System.Text.Encoding.UTF8.GetBytes(mensagemtexto));
                    await output.AddAsync(msg);
                    logger.LogInformation($"Mensagem Enviada: {mensagemtexto}");
                }




                
             
            
            


            
    
    
    
    }
        }
    }


    public class msgmodbus
    {

        public string DisplayName { get; set; }
        public string HwId { get; set; }

        public string Address { get; set; }

        public string Value { get; set; }

        public string SourceTimestamp { get; set; }
   
    }

}
