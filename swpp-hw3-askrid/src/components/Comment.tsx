interface Props {
  authorName: string;
  content: string;
}

const Comment = ({ authorName, content }: Props) => {
  return (
    <article className="text-lg p-4 bg-slate-100 rounded shadow-inner">
      <h3 className="font-semibold">{authorName}</h3>
      <p>{content}</p>
    </article>
  );
};

export default Comment;
