interface Props {
  title: string;
  authorName: string;
  content: string;
}

const ArticleDetail = ({ title, authorName, content }: Props) => {
  return (
    <article className="bg-white p-6 rounded shadow-lg">
      <header className="pb-6 mb-8 border-b border-b-slate-300">
        <h1 id="article-title" className="text-4xl leading-snug text-centr font-semibold mb-2">
          {title}
        </h1>
        <h2 id="article-author" className="text-xl italic">
          {authorName}
        </h2>
      </header>
      <p id="article-content" className="text-justify text-lg">{content}</p>
    </article>
  );
};

export default ArticleDetail;
