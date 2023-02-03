using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.IO.Ports;
using System.Windows.Controls;
using System.Management;
using System.Linq;

namespace OxymetryApp
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        // Serial port
        private SerialPort _serialPort;
        private string _PortName = "";
        private int _spo2_val = 95;
        private byte[] _RxBuffer = new byte[1024];
        private int _offset = 0;
        private int spo2 = 0;
        private int hr = 0;

        public MainWindow()
        {
            InitializeComponent();
            open_uart();
        }

        private void open_uart()
        {
            using (var searcher = new ManagementObjectSearcher
                ("SELECT * FROM WIN32_SerialPort"))
            {
                string[] portnames = SerialPort.GetPortNames();
                if (portnames.Length > 0)
                {
                    _PortName = portnames[0];
                    _serialPort = new SerialPort(_PortName);
                    _serialPort.BaudRate = 115200;
                    _serialPort.Parity = Parity.None;
                    _serialPort.StopBits = StopBits.One;
                    _serialPort.DataBits = 8;

                    _serialPort.DataReceived += new SerialDataReceivedEventHandler(rev_callback);

                    try
                    {
                        _serialPort.Open();
                        _serialPort.DiscardInBuffer();
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.Message);
                    }
                }
            }
        }

        private void rev_callback(object sender, SerialDataReceivedEventArgs e)
        {
            SerialPort sp = (SerialPort)sender;
            int bytes = sp.Read(_RxBuffer, 0, 8);
            if (_RxBuffer[0] == 'S' && _RxBuffer[1] == '_')
            {
                //if((spo2 == 0) || (Math.Abs(spo2 - _RxBuffer[i + 2]) < 10))
                {
                    spo2 = (int)_RxBuffer[2];
                }
                //if ((hr == 0) || (Math.Abs(hr - _RxBuffer[i + 3]) < 50))
                {
                    hr = (int)_RxBuffer[3];
                }
                            
                int PI = (int)_RxBuffer[4];
                App.Current.Dispatcher.Invoke(delegate
                {
                    lbl_SpO2_up.Content = spo2;
                    lbl_SpO2_dn.Content = spo2;
                    lbl_HR.Content= hr;
                    lbl_PI.Content = PI;
                });
            }
        }
        

        private void ContentControl_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            open_uart();
        }
    }
}
