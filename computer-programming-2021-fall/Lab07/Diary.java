import java.time.Year;
import java.util.*;

public class Diary {
    private LinkedList<DiaryEntry> diaryList;
    private HashMap<Integer, Set<String>> searchMap;

	public Diary() {
    	this.diaryList = new LinkedList<>();
        this.searchMap = new HashMap<>();
	}
	
    public void createEntry() {
    	//TODO
        // Practice 1 - Create a diary entry
        // Practice 2 - Store the created entry in a file

        String title = DiaryUI.input("title: ");
        String content = DiaryUI.input("content: ");
        DiaryUI.print("The entry is saved.");

        DiaryEntry e = new DiaryEntry(title, content);
        diaryList.add(e);

        Set<String> s = new HashSet<>();
        s.add(title);
        s.add(content);
        searchMap.put(e.getID(), s);
    }

    public void listEntries(int argNum) {
    	//TODO
        // Practice 1 - List all the entries - sorted in created time by ascending order
        // Practice 2 - Your list should contain previously stored files
        if (argNum == 1) {
            Collections.sort(diaryList, (x, y) -> x.getDateTimeString().compareTo(y.getDateTimeString()));
            for (DiaryEntry e : diaryList) {
                DiaryUI.print(e.getShortString());
            }
        }

        if (argNum == 2) {
            Collections.sort(diaryList, (x, y) -> x.getTitle().compareTo(y.getTitle()));
            for (DiaryEntry e : diaryList) {
                DiaryUI.print(e.getShortString());
            }
        }

        if (argNum == 3) {
            Collections.sort(diaryList, (x, y) -> (x.getTitle().compareTo(y.getTitle()) != 0) ? x.getTitle().compareTo(y.getTitle()) : y.getLength() - x.getLength());
            for (DiaryEntry e : diaryList) {
                DiaryUI.print(e.getShortString() + ", length: " + e.getLength());
            }
        }

        
    }

    public void readEntry(int id) {
    	//TODO
        // Practice 1 - Read the entry of given id
        // Practice 2 - Your read should contain previously stored files

        DiaryEntry e = null;

        for (DiaryEntry d : diaryList) {
            if (d.getID() == id) {
                e = d;
                break;
            }
        }

        if (e != null) {
            DiaryUI.print(e.getFullString());
        } else {
            DiaryUI.print("There is no entry with id " + id + ".");
        }
    }

    public void deleteEntry(int id) {
    	//TODO
        // Practice 1 - Delete the entry of given id
        // Practice 2 - Delete the file of the entry

        DiaryEntry e = null;

        for (DiaryEntry d : diaryList) {
            if (d.getID() == id) {
                e = d;
                break;
            }
        }

        if (e != null) {
            diaryList.remove(e);
            searchMap.remove(id);
            DiaryUI.print("Entry " + id + " is removed.");
        } else {
            DiaryUI.print("There is no entry with id " + id + ".");
        }
    }

    public void searchEntry(String keyword) {
        //TODO
        // Practice 1 - Search and print all the entries containing given keyword
        // Practice 2 - Your search should contain previously stored files
        boolean found = false;

        for (Integer id : searchMap.keySet()) {
            if (searchMap.get(id).contains(keyword)) {
                readEntry(id);
                System.out.printf("\n");
                found = true;
            }
            
        }

        if (!found) {
            DiaryUI.print("There is no entry that contains " + '"' + keyword + '"' + ".");
        }
    }
}
