public class MovieDB {

	private final MyLinkedList<MovieList> genreList;

    public MovieDB() {
		genreList = new MyLinkedList<>();
    }

    public void insert(MovieDBItem item) {
        String newGenre = item.getGenre();
        String newTitle = item.getTitle();

        boolean isNewGenre = true;
        int genreIdx = 0;
        for (; genreIdx < genreList.size(); genreIdx++) {
        	String currGenre = genreList.get(genreIdx).getGenre();
        	if (newGenre.compareTo(currGenre) == 0) {
        		isNewGenre = false;
        		break;
			} else if (newGenre.compareTo(currGenre) < 0) {
				break;
			}
		}
        if (isNewGenre) {
        	MovieList newList = new MovieList(newGenre);
        	genreList.add(genreIdx, newList);
		}



        MovieList targetGenre = genreList.get(genreIdx);
        boolean isNewTitle = true;
        int titleIdx = 0;
        for (; titleIdx < targetGenre.size(); titleIdx++) {
        	String currMovie = targetGenre.get(titleIdx);
        	if (newTitle.compareTo(currMovie) == 0) {
        		isNewTitle = false;
			} else if (newTitle.compareTo(currMovie) < 0) {
        		break;
			}
		}
        if (isNewTitle) {
        	targetGenre.add(titleIdx, newTitle);
		}
    }

    public void delete(MovieDBItem item) {
        String genre = item.getGenre();
        String title = item.getTitle();

        int genreIdx = 0;
        for (; genreIdx < genreList.size(); genreIdx++) {
        	String currGenre = genreList.get(genreIdx).getGenre();
        	if (genre.equals(currGenre))
        		break;
		}
        if (genreIdx >= genreList.size())
        	return;

        MovieList targetGenre = genreList.get(genreIdx);
        for (int titleIdx=0; titleIdx < targetGenre.size(); titleIdx++) {
        	if (title.equals(targetGenre.get(titleIdx)))
        		targetGenre.remove(titleIdx);
		}
        if (targetGenre.isEmpty()) {
        	targetGenre.removeAll();
        	genreList.remove(genreIdx);
		}
    }

    public MyLinkedList<MovieDBItem> search(String term) {
    	MyLinkedList<MovieDBItem> result = new MyLinkedList<>();

    	for (MovieList genre : genreList) {
    		for (String title : genre) {
    			if (title.contains(term)) {
    				MovieDBItem movieItem = new MovieDBItem(genre.getGenre(), title);
    				result.append(movieItem);
				}
			}
		}

        return result;
    }
    
    public MyLinkedList<MovieDBItem> items() {
        MyLinkedList<MovieDBItem> result = new MyLinkedList<>();

        for (MovieList genre : genreList) {
        	for (String title : genre) {
        		MovieDBItem movieItem = new MovieDBItem(genre.getGenre(), title);
        		result.append(movieItem);
			}
		}
        
    	return result;
    }
}

class MovieList extends MyLinkedList<String> {

	public MovieList(String genre) {
		super(genre);
	}

	public String getGenre() {
		return get(-1);
	}
}