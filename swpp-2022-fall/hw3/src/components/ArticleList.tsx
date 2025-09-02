import { useSelector } from "react-redux";
import { Link } from "react-router-dom";
import { selectArticle } from "../store/slices/article";
import ArticleSummary from "./ArticleSummary";

const ArticleList = () => {
  const { articleList } = useSelector(selectArticle);

  return (
    <ul className="grid xl:grid-cols-4 lg:grid-cols-3 sm:grid-cols-2 grid-cols-1 gap-6">
      {articleList.map((article) => (
        <li key={`article-${article.id}`}>
          <Link to={`/articles/${article.id}`}>
            <ArticleSummary id={article.id} title={article.title} authorName={article.author.name} />
          </Link>
        </li>
      ))}
    </ul>
  );
};

export default ArticleList;
