public class NoDataDirectoryException extends Exception {
    
    public NoDataDirectoryException() {
        super("Diary directory data/ is not found.");
    }
}
