import java.util.HashMap;
import java.util.PriorityQueue;
import java.util.Stack;

public class SubwayGraph {
    private final HashMap<String, Node> adjTable;       // key: ID
    private final HashMap<String, Station> nameTable;   // key: name, value: 같은 이름을 가지는 Station 중 처음 저장된 것.

    SubwayGraph() {
        this.adjTable = new HashMap<>();
        this.nameTable = new HashMap<>();
    }

    void addNode(String ID, String name, String line) {
        if (nameTable.containsKey(name)) {     // 같은 이름의 역이 이미 존재할 경우.
            String hubID = Hub.makeID(name);

            if (!adjTable.containsKey(hubID)) { // 허브가 없을 경우 새로 만들고, 이미 있던 역과 연결한다.
                Hub newHub = new Hub(hubID);
                Station oldSt = nameTable.get(name);
                adjTable.put(hubID, newHub);
                newHub.update(oldSt, 0);
                oldSt.update(newHub, 5);
            }
            // 새로운 역을 만들고, 허브와 연결.
            Hub hub = (Hub) adjTable.get(hubID);
            Station newSt = new Station(ID, name, line);
            adjTable.put(ID, newSt);
            hub.update(newSt, 0);
            newSt.update(hub, 5);
        } else {    // 환승이 없는 역은 허브를 만들지 않는다.
            Station newSt = new Station(ID, name, line);
            adjTable.put(ID, newSt);
            nameTable.put(name, newSt);
        }
    }

    void connectNode(String startID, String endID, int distance) {
        Node startNode = adjTable.get(startID);
        Node endNode = adjTable.get(endID);
        startNode.update(endNode, distance);
    }

    void printPath(String startName, String endName) throws NullPointerException {
        int startToEnd = 0;
        Node startNode = nameTable.get(startName).getPoint();
        Node endNode = nameTable.get(endName).getPoint();

        // 도착지에서 hub로 이동하는 시간은 제외한다.
        if (endNode instanceof Hub) {
            startToEnd -= 5;
        }

        PriorityQueue<Dst> minHeap = new PriorityQueue<>(); // 새 거리가 계산되면 minHeap에 새로 저장.
        HashMap<Node, Integer> distTable = new HashMap<>(); 
        HashMap<Node, Node> pathTable = new HashMap<>();
        
        minHeap.add(new Dst(startNode, 0));
        distTable.put(startNode, 0);

        while (!minHeap.isEmpty()) {
            Dst temp = minHeap.poll();
            Node currNode = temp.node;
            int currDist = temp.distance;

            if (currNode == endNode) {
                startToEnd += currDist;
                break;
            }

            if (currDist > distTable.get(currNode)) {
                continue;
            }

            for (Dst dst : currNode.adj.values()) {
                if (distTable.containsKey(dst.node)) {      // dst.node가 이미 방문된 노드라면...
                    if ((currDist + dst.distance) < distTable.get(dst.node)) {  // 새로 계산된 거리가 기존보다 더 작으면...
                        minHeap.add(new Dst(dst.node, currDist + dst.distance));
                        distTable.put(dst.node, currDist + dst.distance);
                        pathTable.put(dst.node, currNode);
                    }
                    
                } else {
                    minHeap.add(new Dst(dst.node, currDist + dst.distance));
                    distTable.put(dst.node, currDist + dst.distance);
                    pathTable.put(dst.node, currNode);
                }
            }
        }

        Stack<Station> pathStk = new Stack<>();

        Node e = endNode;
        while (true) {
            if (e instanceof Station) {
                pathStk.push((Station) e);
            }

            if (pathTable.containsKey(e)) {
                e = pathTable.get(e);
            } else {
                break;
            }
        }

        Station cur;
        Station nxt;
        while (true) {
            cur = pathStk.pop();
            if (!pathStk.isEmpty()) {
                nxt = pathStk.peek();
                if (cur.name.compareTo(nxt.name) == 0) {
                    pathStk.pop();
                    System.out.printf("[%s] ", cur.name);
                } else {
                    System.out.printf("%s ", cur.name);
                }
            } else {
                System.out.printf("%s", cur.name);
                break;
            }
        }

        System.out.println();
        System.out.println(startToEnd);
    }
}

class Node {
    String ID;
    HashMap<String, Dst> adj;   // key: ID

    void update(Node node, int distance) {
        if (adj.containsKey(node.ID)) {
            adj.get(node.ID).distance = distance;
        } else {
            Dst newDst = new Dst(node, distance);
            adj.put(node.ID, newDst);
        }
    }
}

class Station extends Node {
    String name;
    String line;
    
    Station(String ID, String name, String line) {
        this.ID = ID;
        this.name = name;
        this.line = line;
        this.adj = new HashMap<>();
    }

    // hub가 있으면 hub를 리턴한다. 없으면 자기자신을 리턴.
    Node getPoint() {
        String hubID = Hub.makeID(name);
        if (adj.containsKey(hubID)) {
            return adj.get(hubID).node;
        } else {
            return this;
        }
    }
}

class Hub extends Node {
    Hub(String ID) {
        this.ID = ID;
        this.adj = new HashMap<>();
    }

    static String makeID(String name) {
        return "HUB" + name;
    }
}

class Dst implements Comparable<Dst> {
    Node node;
    int distance;

    Dst(Node node, int distance) {
        this.node = node;
        this.distance = distance;
    }

    public int compareTo(Dst other) {
        return Integer.compare(this.distance, other.distance);
    }
}