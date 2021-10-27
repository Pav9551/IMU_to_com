
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO.Ports;
using System.IO;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using System.Management;
using Microsoft.Win32;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Media;
namespace WindowsFormsApp1
{
    public class Class_data
    {
        public SerialPort serialPort1;
        public int state_of_port;

        private static ushort calcrc(byte[] data, int len_h, int len_crc)
        {
            ushort wCRC = 0xffff;
            for (int i = len_h; i < (data.Length - len_crc); i++)
            {
                wCRC ^= (ushort)(data[i] << 8);
                for (int j = 0; j < 8; j++)
                {
                    if ((wCRC & 0x8000) != 0)
                        wCRC = (ushort)((wCRC << 1) ^ 0x1021);
                    else
                        wCRC <<= 1;
                }
            }
            return wCRC;
        }



        public struct MPU
        {
            public double time;
            public int err;
            public double ax;
            public double ay;
            public double az;
            public double wx;
            public double wy;
            public double wz;
            public float q0;
            public float q1;
            public float q2;
            public float q3;

            public float e0;
            public float e1;
            public float e2;

            public float angle;
            public float angle_old;
            public float count_360;

            public int count;
            public int count_old;
            public ushort crc;

        }
        [StructLayout(LayoutKind.Explicit, Pack = 1)]
        public struct in_data_MPU6050
        {
            [FieldOffset(0)] public ushort header;

            [FieldOffset(2)] public int Wx;
            [FieldOffset(6)] public int Wy;
            [FieldOffset(10)] public int Wz;

            [FieldOffset(14)] public int Ax;
            [FieldOffset(18)] public int Ay;
            [FieldOffset(22)] public int Az;

            [FieldOffset(26)] public ushort ADD;
            [FieldOffset(28)] public byte counter;
            [FieldOffset(29)] public byte st;
            [FieldOffset(30)] public ushort crc;

        }
        public MPU out_data;
        public in_data_MPU6050 in_data;

        public Class_data(SerialPort serialPort1)
        {
            //настройка порта
            if (serialPort1.IsOpen) serialPort1.Close();
            this.serialPort1 = serialPort1;
            this.serialPort1.BaudRate = 460800;//460800
            this.serialPort1.DataBits = 8;
            this.serialPort1.StopBits = StopBits.One;
            this.serialPort1.Parity = Parity.None;
            this.serialPort1.Open();
            if (this.serialPort1.IsOpen)
            {
                //  MessageBox.Show("Порт удачно открыт!", "открытие порта", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else
                MessageBox.Show("Порт невозможно открыть!", "0000", MessageBoxButtons.OK, MessageBoxIcon.Warning);

           
    }
        public void close_port()

        {
            this.serialPort1.Close();
            this.serialPort1.Dispose();
        }

        public void clear_out()

        {

            this.out_data.ax = 0;
            this.out_data.ay = 0;
            this.out_data.az = 0;

            this.out_data.wx = 0;
            this.out_data.wy = 0;
            this.out_data.wz = 0;

            this.out_data.time = 0.0028;

            /*this.out_data.q0 = 1.0f;  
            this.out_data.q1 = 0f;
            this.out_data.q2 = 0f;
            this.out_data.q3 = 0f;*/

            this.out_data.q0 = 0.70710f;// Поворот вокруг X на 90
            this.out_data.q1 = -0.70710f;
            this.out_data.q2 = 0f;
            this.out_data.q3 = 0f;

            this.out_data.e0 = 0f;
            this.out_data.e1 = 0f;
            this.out_data.e2 = 0f;

            this.out_data.count_360 = 0;
            this.out_data.angle_old = 0;
        }


        public void portUpdate()
        {

            double DEG_TO_RAD = 3.141592653589793238463 / 180.0;
            while (this.serialPort1.BytesToRead > 0)
            {
                byte[] buff = { 0xc0, 0xc0, 0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x89, 0xff, 0x02, 0x00, 0xfe, 0x06, 0x00, 0x00, 0x7e, 0x02, 0x00, 0x00, 0x45, 0x12, 0xbd, 0x60, 0x1d, 0xf1 };
            //byte[] buff1 = { 0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x89, 0xff, 0x02, 0x00, 0xfe, 0x06, 0x00, 0x00, 0x7e, 0x02, 0x00, 0x00, 0x45, 0x12, 0xbd, 0x60 };
            int b = this.serialPort1.ReadByte();

            if (b != 0xc0) continue;
            b = this.serialPort1.ReadByte();
            if (b != 0xc0) continue;
                //textBox1.AppendText(b.ToString("X2") + "\n");

            this.serialPort1.Read(buff, 2, 30);
            GCHandle handle;

            handle = GCHandle.Alloc(buff, GCHandleType.Pinned);
            in_data = (in_data_MPU6050)Marshal.PtrToStructure(handle.AddrOfPinnedObject(), typeof(in_data_MPU6050));
            handle.Free();
            int size = Marshal.SizeOf(in_data);
            ushort wCRC = calcrc(buff, 2, 2);
                if ((buff[30] == ((wCRC) & 0xFF)) & (buff[31] == ((wCRC >> 8) & 0xFF)))
                {
                    out_data.ax = (double)(in_data.Ax - 218) / 16384 * 9.81;//blue
                    out_data.ay = (double)(in_data.Ay - 80) / 16384 * 9.81;
                    out_data.az = (double)(in_data.Az + 296) / 16384 * 9.81;
                    out_data.wx = (double)(in_data.Wx - 38) / 131 * DEG_TO_RAD;
                    out_data.wy = (double)(in_data.Wy - 234) / 131 * DEG_TO_RAD;
                    out_data.wz = (double)(in_data.Wz + 150) / 131 * DEG_TO_RAD;
                    out_data.time = (double)in_data.ADD / 1000000;
                    out_data.crc = in_data.crc;
                }
            }

        }
    }
}