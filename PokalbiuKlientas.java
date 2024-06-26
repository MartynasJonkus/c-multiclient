import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.*;
import java.net.*;

import javax.swing.*;

/**
 * Klientas:
 * 1) gauna pranešimą "ATSIUSK_VARDA" iš serverio (tol, kol nebus tinkamo
 * vardo);
 * 2) jeigu vardas priimtas, gauna iš serverio "VARDAS_OK";
 * 3) ... tada gali rašyti pranešimus, kuriuos serveris dalina visiems
 */
public class PokalbiuKlientas {

    BufferedReader ivestis;
    PrintWriter isvestis;
    JFrame langas = new JFrame("Pokalbiai");
    JTextField tekstoLaukelis = new JTextField(40);
    JTextArea pranesimuSritis = new JTextArea(8, 40);

    public PokalbiuKlientas() {
        tekstoLaukelis.setEditable(false); // kol negavome neprisijugėme
        pranesimuSritis.setEditable(false); // kol negavome neprisijugėme
        langas.getContentPane().add(tekstoLaukelis, "North");
        langas.getContentPane().add(new JScrollPane(pranesimuSritis), "Center");
        langas.pack();

        tekstoLaukelis.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                isvestis.println(tekstoLaukelis.getText());
                tekstoLaukelis.setText("");
            }
        });
    }

    private String koksServerioAdresasIrPortas() {
        return JOptionPane.showInputDialog(
                langas,
                "Serverio IP ir portas?",
                "Klausimas",
                JOptionPane.QUESTION_MESSAGE);
    }

    private String koksVardas() {
        return JOptionPane.showInputDialog(
                langas,
                "Pasirink vardą:",
                "Vardas...",
                JOptionPane.PLAIN_MESSAGE);
    }

    private void run() throws IOException {

        // Prisijungiame ir inicializuojame I/O...
        String serverioAdresasIrPortas = koksServerioAdresasIrPortas();
        String[] info = serverioAdresasIrPortas.split("[ ]+");
        Socket soketas = new Socket(info[0], Integer.parseInt(info[1]));
        ivestis = new BufferedReader(new InputStreamReader(soketas.getInputStream()));
        isvestis = new PrintWriter(soketas.getOutputStream(), true);

        while (true) {
            String tekstas = ivestis.readLine();
            System.out.println("Serveris ---> Klientas : " + tekstas);
            if (tekstas.startsWith("ATSIUSKVARDA")) {
                isvestis.println(koksVardas());
            } else if (tekstas.startsWith("VARDASOK")) {
                tekstoLaukelis.setEditable(true);
            } else {
                pranesimuSritis.append(tekstas + "\n");
            }
        }
    }

    public static void main(String[] args) throws Exception {
        PokalbiuKlientas klientas = new PokalbiuKlientas();
        klientas.langas.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        klientas.langas.setVisible(true);
        klientas.run();
    }
}
