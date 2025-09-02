import java.io.*;

public class Subway
{
    private final static SubwayGraph graph = new SubwayGraph();
	public static void main(String [] args) {
        File file = new File(args[0]);
        String input;
        int sel = 0;

        try (BufferedReader br = new BufferedReader(new FileReader(file))) {
            while ((input = br.readLine()) != null) {
                if (input.isEmpty()) {
                    sel = 1;
                } else {
                    command(input, sel);
                }
            }
        } catch (Exception e){
            System.out.println("파일 입력이 잘못되었습니다.");
        }
        sel = 2;

        BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
        while (true) {
            try {
                input = br.readLine();
                if (input.compareTo("QUIT") == 0) {
                    break;
                }
                command(input, sel);
            } catch (Exception e) {
                System.out.println("입력이 잘못되었습니다.");
            }
        }
    }

	private static void command(String input, int sel) throws Exception{
        String[] inputArr = input.split(" ");
        switch (sel) {
            case 0:
                String ID = inputArr[0];
                String name = inputArr[1];
                String line = inputArr[2];
                graph.addNode(ID, name, line);
                break;
            case 1:
                String startID = inputArr[0];
                String endID = inputArr[1];
                int distance = Integer.parseInt(inputArr[2]);
                graph.connectNode(startID, endID, distance);
                break;
            case 2:
                String startName = inputArr[0];
                String endNAme = inputArr[1];
                graph.printPath(startName, endNAme);
                break;
        }
	}
}
