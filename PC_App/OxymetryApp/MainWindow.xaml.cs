using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace OxymetryApp
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private int _spo2_val = 95;
        public MainWindow()
        {
            InitializeComponent();
            PeriodicFunc(updateData, TimeSpan.FromSeconds(1), TimeSpan.FromSeconds(1), CancellationToken.None);
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
