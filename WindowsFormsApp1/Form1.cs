using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO.Ports;


namespace WindowsFormsApp1
{
    
    public partial class Form_test_com : Form
    {
        public Form_test_com()
        {
            InitializeComponent();
            
        }
        public Class_data data;//объявляем экземпляр класса
        private void Form_test_com_Load(object sender, EventArgs e)
        {
            string[] ports = SerialPort.GetPortNames();
            foreach (string port in ports)
            {
                combo_Box_port.Items.Add(port);
                combo_Box_port.Text = ports[0];
            }
            

        }

        private void portButton_Click(object sender, EventArgs e)
        {
            //data.state_of_port = 0;
            if (portButton.Text == "Open")
            {
                if (serialPort1.IsOpen) serialPort1.Close();

                serialPort1.PortName = combo_Box_port.Text;
                try
                {
                    data = new Class_data(serialPort1);// создаем экземпляр класса  
                    
                    data.clear_out();//очищаем выходные данные
                    portButton.Text = "Close";
                    timer1.Enabled = true;
                    Application.DoEvents();
                    data.state_of_port = 1;//

                }
                catch
                {
                    MessageBox.Show("Порт " + serialPort1.PortName +
                        " невозможно открыть!", "Ошибка!", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    combo_Box_port.SelectedText = "";
                    portButton.Text = "Open";

                }
            }
            else
            {
                if (serialPort1.IsOpen)
                {
                    
                    timer1.Enabled = false;
                    Application.DoEvents();

                    serialPort1.Close();
                    serialPort1.Dispose();
                  
                    data.state_of_port = 0;//
                    portButton.Text = "Open";
                }
                
                
               
            }
        }

        private void serialPort1_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            if (timer1.Enabled) this.Invoke(new EventHandler(DoUpdate));
        }

        private void DoUpdate(object s, EventArgs e)
        {

            if (timer1.Enabled) data.portUpdate();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            label1.Text = "Ax = " + data.out_data.ax.ToString("F2");
            //label2.Text = data.state_of_port.ToString("F0");
        }

        private void Form_test_com_FormClosed(object sender, FormClosedEventArgs e)
        {
            timer1.Enabled = false;
            Application.DoEvents();
            /* if (fstream != null)
                 fstream.Close();
             if (sw != null)
                 sw.Close();*/
            portButton.Text = "Close";
            Application.DoEvents();
            portButton_Click(sender, e);
            //if (dis == 0) portButton_Click(sender, e);
            Application.DoEvents();
            Application.Exit();
        }

       
    }
}
