using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
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
using System.Net; // Librerías necesarias
using System.Net.Sockets; // Librerías necesarias

namespace Practica3
{
    public partial class MainWindow : Window
    {
        // Definición de variables
        float Kp;
        float Ki;
        float Ts;
        ushort Consigna;
        public MainWindow()
        {
            InitializeComponent();
            InitializeTimer();
        }
        // Temporizador para enviar los datos periódicamente
        void InitializeTimer()
        {
            timer = new System.Windows.Threading.DispatcherTimer();
            timer.Tick += new EventHandler(timer_Tick);
            timer.Interval = new TimeSpan(0, 0, 0, 1, 0);
            timer.Start();// Arranca el temporizador 
        }
        System.Windows.Threading.DispatcherTimer timer;

        // Con el evento que genera el timer, se envían los datos
        private void timer_Tick(object sender, EventArgs e)
        {
            DataSend();
        }
        // Con la pulsación del botón de envío de consigna se actualizan los valores de Kp, Ki, Ts y Consigna
        private void Boton_Consigna_Click(object sender, RoutedEventArgs e)
        {
            Kp = Convert.ToSingle(textBox_P.Text);
            Ki = Convert.ToSingle(textBox_I.Text);
            Ts = Convert.ToSingle(textBox_Ts.Text);
            Consigna = Convert.ToUInt16(Slider_Consigna.Value);
        }
        // Envío de datos periódico cada segundo con el evento del temporizador
        private void DataSend()
        {
            try
            {
                IPEndPoint SAPServidor = new IPEndPoint(IPAddress.Parse(editorIP.Text),
                 Convert.ToInt32(editorPuerto.Text));
                // Objeto para expresar la dirección IP y puerto donde podemos localizar al
                // sistema embebido
                
                Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram,
                ProtocolType.Udp);
                // Crea un socket para manejar la comunicación bajo protocolo UDP
               
                socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReceiveTimeout, 300);
                // Las operaciones de recepción tienen un timeout de 300 milisegundos. Si una operación
                // de recepción tarda más, genera una excepción

                byte[] bytesKp = BitConverter.GetBytes(Kp);
                byte[] bytesKi = BitConverter.GetBytes(Ki);
                byte[] bytesTs = BitConverter.GetBytes(Ts);
                byte[] bytesConsigna = BitConverter.GetBytes(Consigna);
                // Convierte los datos a matrices de bytes
                
                byte[] paqueteEnvio = new byte[4 + 4 + 4 + 2];
                // Creamos el paquete de bytes a enviar al sistema embebido 
                
                for (int i = 0; i < 4; i++)
                    paqueteEnvio[i] = bytesKp[i];
                for (int i = 0; i < 4; i++)
                    paqueteEnvio[i + 4] = bytesKi[i];
                for (int i = 0; i < 4; i++)
                    paqueteEnvio[i + 8] = bytesTs[i];
                for (int i = 0; i < 2; i++)
                    paqueteEnvio[i + 12] = bytesConsigna[i];
                // Coloca los bytes de Kp,Ki,Ts y Consigna en el paquete
               
                socket.SendTo(paqueteEnvio, 4 + 4 + 4 + 2, SocketFlags.None, SAPServidor);
                // Envía el paquete al sistema embebido
                
                byte[] paqueteRecepcion = new byte[4 + 4];
                IPEndPoint SAPClienteValida = new IPEndPoint(IPAddress.Any, 0);
                EndPoint SAPCliente = (EndPoint)SAPClienteValida;
                socket.ReceiveFrom(paqueteRecepcion, 4 + 4, SocketFlags.None, ref SAPCliente);
                // Recibe la respuesta en 'paqueteRecepcion', de 8 bytes
                
                float lecturaActual = BitConverter.ToSingle(paqueteRecepcion, 0);
                float actuacionActual = BitConverter.ToSingle(paqueteRecepcion, 4);
                // Extrae los 4 bytes de la lectura actual y los 4 bytes de la actuación actual
               
                LabelLectura.Content = "Lectura = " + lecturaActual;
                LabelActuacion.Content = "Actuación = " + actuacionActual;
                // Muestra por pantalla la lectura y la actuación
            }
            catch (Exception ex)
            { // Si salta alguna excepción ...
                MessageBox.Show(ex.Message, "Error comunicación UDP");
              // Muestra un mensaje de error en otra ventana
            }
        }
       
    }
}

