import { useMemo } from "react";
import { useSelector } from "react-redux";
import { selectArticle } from "../store/slices/article";
import { selectAuth } from "../store/slices/auth";
import ArticleDetail from "./ArticleDetail";
import ArticleDeleteButton from "./buttons/ArticleDeleteButton";
import BackToArticlesButton from "./buttons/BackToArticlesButton";
import GoToArticleEditButton from "./buttons/GoToArticleEditButton";

const ArticleDetailView = () => {
  const { selectedArticle } = useSelector(selectArticle);
  const { claims } = useSelector(selectAuth);

  const isAuthor = useMemo(() => selectedArticle?.author.id === claims?.id, [selectedArticle, claims]);

  return (
    <div>
      <div className="flex justify-between mb-2">
        <div className="space-x-2">
          {isAuthor && (
            <>
              <GoToArticleEditButton />
              <ArticleDeleteButton />
            </>
          )}
        </div>
        <BackToArticlesButton elementId="back-detail-article-button" />
      </div>
      <ArticleDetail
        title={selectedArticle?.title ?? ""}
        authorName={selectedArticle?.author.name ?? ""}
        content={selectedArticle?.content ?? ""}
      />
    </div>
  );
};

export default ArticleDetailView;
