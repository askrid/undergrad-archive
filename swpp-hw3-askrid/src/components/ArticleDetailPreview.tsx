import { useSelector } from "react-redux";
import { selectArticle } from "../store/slices/article";
import { selectAuth } from "../store/slices/auth";
import ArticleDetail from "./ArticleDetail";

const ArticleDetailPreview = () => {
  const { writingArticle } = useSelector(selectArticle);
  const { claims } = useSelector(selectAuth);

  return (
    <ArticleDetail
      title={writingArticle.title}
      content={writingArticle.content}
      authorName={claims?.name!}
    />
  );
};

export default ArticleDetailPreview;
