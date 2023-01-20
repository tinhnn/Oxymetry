﻿using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.IO.Ports;


namespace OxymetryApp
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        // Serial port
        private SerialPort _serialPort;
        private int _spo2_val = 95;
        private byte[] _RxBuffer = new byte[1024];
        private int _offset = 0;

        public MainWindow()
        {
            InitializeComponent();
            open_uart();
            //PeriodicFunc(updateData, TimeSpan.FromSeconds(1), TimeSpan.FromSeconds(1), CancellationToken.None);
        }

        private void open_uart()
        {
            _serialPort = new SerialPort("COM3");
            _serialPort.BaudRate = 115200;
            _serialPort.Parity = Parity.None;
            _serialPort.StopBits = StopBits.One;
            _serialPort.DataBits = 8;

            _serialPort.DataReceived += new SerialDataReceivedEventHandler(rev_callback);

            
            _serialPort.Open();
            _serialPort.DiscardInBuffer();
        }

        private void rev_callback(object sender, SerialDataReceivedEventArgs e)
        {
            SerialPort sp = (SerialPort)sender;
            int bytes = sp.Read(_RxBuffer, _offset, 8);
            _offset += bytes;
            if(_offset >= 8)
            {
                for(int i = 0; i < _offset-3; i++)
                {
                    if (_RxBuffer[i] == 'S' && _RxBuffer[i+1] == '_')
                    {
                        int spo2 = (int)_RxBuffer[i+2];
                        int hr = (int)_RxBuffer[i+3];
                        int PI = (int)_RxBuffer[i+4];
                        App.Current.Dispatcher.Invoke(delegate
                        {
                            lbl_SpO2_up.Content = spo2;
                            lbl_SpO2_dn.Content = spo2;
                            lbl_HR.Content= hr;
                            lbl_PI.Content = PI;
                        });
                        
                    }
                }
            }
        }
        private async Task PeriodicFunc(Action ontick, TimeSpan duetime, TimeSpan interval, CancellationToken token)
        {
            if(duetime > TimeSpan.Zero)
            {
                await Task.Delay(duetime, token);
            }

            while(!token.IsCancellationRequested)
            {
                ontick?.Invoke();
                if(interval> TimeSpan.Zero)
                {
                    await Task.Delay(interval, token);
                }
            }
        }
        private void updateData()
        {
            lbl_SpO2_up.Content = _spo2_val;
            lbl_SpO2_dn.Content = _spo2_val;
            lbl_HR.Content = 70;

            if (++_spo2_val > 100)
            {
                _spo2_val = 95;
            }
        }
    }
}
