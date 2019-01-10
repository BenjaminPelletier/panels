using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace DmxServer
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length != 1 && args.Length != 2)
            {
                Console.Error.WriteLine("Usage: DmxServer <COMM PORT> [<HTTP PORT>]");
                return;
            }

            string dmxPort = args[0];
            Console.WriteLine("Connecting to " + dmxPort);
            var dmx = new DmxController(dmxPort);

            // https://stackoverflow.com/a/27376368/651139
            string localIP;
            using (Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, 0))
            {
                socket.Connect("8.8.8.8", 65530);
                IPEndPoint endPoint = socket.LocalEndPoint as IPEndPoint;
                localIP = endPoint.Address.ToString();
            }

            var listener = new HttpListener();
            string prefix = "http://" + localIP + ":" + (args.Length > 1 ? args[1] : "8080") + "/";
            listener.Prefixes.Add(prefix);
            Console.WriteLine("Listening at " + prefix);
            while (true)
            {
                listener.Start();
                var context = listener.GetContext();
                var request = context.Request;
                var response = context.Response;

                Result result;
                if (request.HttpMethod == "GET")
                {
                    if (request.Url.AbsolutePath == "/dmx")
                    {
                        result = ExecuteGet(dmx, request);
                    }
                    else if (request.Url.AbsolutePath == "/status")
                    {
                        result = new Result { Code = HttpStatusCode.OK, Content = "DMX ready on " + dmxPort };
                    }
                    else
                    {
                        result = new Result { Code = HttpStatusCode.NotFound, Content = "Path " + request.Url.AbsolutePath + " not found" };
                    }
                }
                else if (request.HttpMethod == "POST")
                {
                    if (request.Url.AbsolutePath == "/dmx")
                    {
                        result = ExecutePut(dmx, request);
                    }
                    else
                    {
                        result = new Result { Code = HttpStatusCode.NotFound, Content = "Path " + request.Url.AbsolutePath + " not found" };
                    }
                }
                else
                {
                    result = new Result
                    {
                        Code = HttpStatusCode.MethodNotAllowed,
                        Content = request.HttpMethod + " not supported"
                    };
                }

                response.StatusCode = (int)result.Code;
                var buffer = Encoding.UTF8.GetBytes("<html><body>" + result.Content + "</body></html>");
                response.ContentLength64 = buffer.Length;
                var output = response.OutputStream;
                output.Write(buffer, 0, buffer.Length);
                output.Close();
            }
        }

        static Result ExecuteGet(DmxController dmx, HttpListenerRequest request)
        {
            string error = "";
            for (int i = 0; i < request.QueryString.Count; i++)
            {
                string key = request.QueryString.GetKey(i);
                string[] values = request.QueryString.GetValues(i);
                string name = "'" + key + "=" + values[values.Length - 1] + "'";
                if (key.StartsWith("ch"))
                {
                    int channel;
                    if (!int.TryParse(key.Substring(2), out channel))
                    {
                        error = "Could not parse channel in argument " + name;
                        break;
                    }
                    if (channel < 0 || channel >= 512)
                    {
                        error = "Invalid channel in argument " + name;
                        break;
                    }
                    int value;
                    if (!int.TryParse(values[values.Length - 1], out value))
                    {
                        error = "Could not parse DMX value in argument " + name;
                        break;
                    }
                    if (value < 0 || value > 255)
                    {
                        error = "Invalid DMX value in argument " + name;
                        break;
                    }
                    try
                    {
                        dmx[channel] = (byte)value;
                    } catch (Exception ex)
                    {
                        return new Result
                        {
                            Code = HttpStatusCode.InternalServerError,
                            Content = "Error while setting DMX channel " + channel + " to " + value + ": " + ex.Message
                        };
                    }
                }
                else
                {
                    error = "Unrecognized argument: " + name + "<br>";
                    break;
                }
            }
            return new Result
            {
                Content = error.Length > 0 ? error : "OK",
                Code = error.Length > 0 ? HttpStatusCode.BadRequest : HttpStatusCode.OK
            };
        }

        static Dictionary<byte, int> DIGITS = MakeDigits();
        static Dictionary<byte, int> MakeDigits()
        {
            var result = new Dictionary<byte, int>();
            for (int i = 0; i < 16; i++)
            {
                result[(byte)("0123456789ABCDEF"[i])] = i;
                result[(byte)("0123456789abcdef"[i])] = i;
            }
            return result;
        }

        static Result ExecutePut(DmxController dmx, HttpListenerRequest request)
        {
            if (!request.HasEntityBody || request.ContentLength64 != 1024)
            {
                return new Result
                {
                    Code = HttpStatusCode.BadRequest,
                    Content = "Expected 1024 ASCII characters encoding 512 bytes of data in request body"
                };
            }
            long n = request.ContentLength64;
            var buffer = new byte[1024];
            request.InputStream.Read(buffer, 0, buffer.Length);

            byte[] newData;
            try
            {
                newData = Enumerable.Range(0, 512).Select(i => (byte)(DIGITS[buffer[2 * i]] * 0x10 + DIGITS[buffer[2 * i + 1]])).ToArray();
            } catch (Exception ex)
            {
                return new Result
                {
                    Code = HttpStatusCode.BadRequest,
                    Content = ex.Message
                };
            }

            try
            {
                dmx.Data = newData;
            } catch (Exception ex)
            {
                return new DmxServer.Program.Result
                {
                    Code = HttpStatusCode.InternalServerError,
                    Content = "Error setting DMX block data: " + ex.Message
                };
            }

            return new Result
            {
                Code = HttpStatusCode.OK,
                Content = "OK"
            };
        }

        struct Result
        {
            public HttpStatusCode Code;
            public string Content;
        }

        class DmxController : IDisposable
        {
            private SerialPort _Serial;
            private Thread _SendLoop;
            private readonly byte[] _DmxData;

            public DmxController(string portName)
            {
                _Serial = new SerialPort(portName, 250000, Parity.None, 8, StopBits.Two);
                _Serial.Open();

                _DmxData = new byte[512];

                _SendLoop = new Thread(this.SendLoop);
                _SendLoop.IsBackground = true;
                _SendLoop.Start();
            }

            public byte this[int channel]
            {
                get
                {
                    lock (_DmxData)
                    {
                        return _DmxData[channel];
                    }
                }
                set
                {
                    lock (_DmxData)
                    {
                        if (channel == 0 && value != 0)
                        {
                            throw new ArgumentException("DMX channel 0 may only have value 0");
                        }
                        _DmxData[channel] = value;
                    }
                }
            }

            public byte[] Data
            {
                set
                {
                    lock (_DmxData)
                    {
                        if (value[0] != 0)
                        {
                            throw new ArgumentException("DMX channel 0 may only have value 0");
                        }
                        Array.Copy(value, _DmxData, _DmxData.Length);
                    }
                }
            }

            private void SendLoop()
            {
                while (true)
                {
                    try
                    {
                        _Serial.BreakState = true;
                        _Serial.BreakState = false;
                        lock (_DmxData)
                        {
                            _Serial.Write(_DmxData, 0, _DmxData.Length);
                        }
                        Thread.Sleep(30);
                    }
                    catch (ThreadAbortException)
                    {
                        break;
                    }
                    catch (Exception e)
                    {
                        Console.Error.WriteLine(e);
                        break;
                    }
                }
            }

            public void Dispose()
            {
                _Serial.Dispose();
            }
        }
    }
}
