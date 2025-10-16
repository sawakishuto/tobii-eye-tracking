#include <windows.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include "tobii_research.h"
#include "tobii_research_streams.h"

static std::atomic<float> g_gazeX{ -1.f };
static std::atomic<float> g_gazeY{ -1.f };
static std::atomic<bool> g_gazeUpdated{ false };

// 統合版：すべてのレポート（32段落 = 8文章 × 4段落）
static const std::vector<std::wstring> g_sections = {
    // 文章1：データ分析研修レポート
    L"会社のデジタル化を円滑に進めるため、新しいデータ分析やAIの活用方法を\n基礎から学ぶ研修に参加しました。これまでは個人の経験や勘に頼って判断する\n場面が多く、スピードや正確性に課題を感じていました。今回の研修を通じて、\n数字という客観的な根拠に基づいて物事を判断できるようになり、そのための\n土台を築きたいと考えました。",
    L"研修は3日間にわたって行われ、前半で統計の基本的な考え方や最新ツールの\n使い方を学び、後半では会社の実際のデータを使って分析の練習をしました。\n特に、Pythonというプログラミング言語と、データをグラフなどで分かりやすく\n見せるBIツールを連携させ、マーケティング部門が持つ見込み顧客の情報を分析した\n実習は、自社の営業活動に直接役立つ内容で、とても理解が深まりました。",
    L"研修を受けたことで、「なぜこの分析手法を選んだのか」を筋道を立てて説明\nできるようになり、上司への報告書を以前よりスムーズに作成できるようになり\nました。また、研修で作った分析画面（ダッシュボード）の試作品を部署内で\n共有したところ、他の部署からも「ぜひ活用したい」という声が上がりました。\nこれまで感覚的な意見交換になりがちだった会議で、具体的なデータを示すことで、\nスムーズに結論を出せるようになった点は大きな成果だと感じています。",
    L"学んだ知識を今後の仕事でしっかりと活かしていくことが次の課題です。まずは、\n研修で作成した分析画面を、いつでも社内システムで見られるように環境を整え、\n誰が閲覧できるかといった運用ルールを整備します。その上で、分析ツールを\n使える人とそうでない人の差をなくすため、実際に操作しながら学べる社内勉強会を\n企画し、部署全体のスキルアップを図っていきたいと考えています。",
    
    // 文章2：業務報告改善プロジェクト
    L"毎日・毎週・毎月の業務報告には、チーム内での情報共有をスムーズにし、\n仕事の偏りをなくし、目標の進み具合を分かりやすくする役割があります。\n今回、報告のやり方を見直した理由は、担当するプロジェクトが増えて仕事の\n重複や抜け漏れが多くなったことと、リモートワークによって気軽に\nコミュニケーションを取る機会が減ったためです。そこで、報告のフォーマットを\n統一し、経営層から現場の担当者まで、みんなが同じ情報を見て判断できる\n仕組みを作ることにしました。",
    L"新しいフォーマットでは、日報に「その日の作業時間と完成したもの」\n「目標（KPI）の達成度」「心配な点や問題になりそうなこと」を書く欄を\n設けました。週報では、毎日の報告から大事な点を抜き出し、数字がどう\n変化したかを分かりやすくまとめます。月報では、その月の目標達成度を\n振り返り、翌月に力を入れることを決めます。この仕組みによって、毎日\nきちんと記録し、週ごとには要点をまとめ、月ごとにはより広い視野で\n考える、というサイクルが身につくように設計しました。",
    L"この新しいやり方を始めて1ヶ月で、上司が報告を確認するためにかかって\nいた時間が3分の2に短縮され、報告ミスによるやり直しの作業も約20%減り\nました。特に、「心配な点」を早めに共有できたことがうまくいき、取引先の\n納品遅れに前もって気づき、別の対策を準備できたことは大きな成果です。\nメンバーからも「自分の仕事の成果が数字で見えるので、やる気につながる」\nという声が聞かれ、チーム全体の仕事への意欲も高まりました。",
    L"現状では手で入力することが多く、数字をまとめるのに時間がかかっているため、\nこれを自動化することが次の課題です。具体的には、今使っているタスク管理\nシステムと連携させ、作業時間などを自動で取り込めるようにします。また、\n目標の決め方が部署によって違うため、会社全体で共通の目標を設定し、部署を\nまたいだ比較や分析ができるようにする計画です。最終的には、分析画面\n（ダッシュボード）でいつでも最新の進捗状況を見られるようにし、経営会議\nなどでもすぐに情報共有できる環境を目指します。",
    
    // 文章3：入札不正問題への対応
    L"2025年3月20日、当社の営業担当Hさんが、地方自治体の入札で、ライバル会社の\n見積もり金額を事前に入手し、社内チャットで共有するという問題が起きました。\nそして、その情報をもとにライバル社より1%だけ安い金額で見積もりを提出して\nいたことが発覚しました。これは法律や会社のルールに違反する重大な不正行為\nです。",
    L"この問題は、入札先である自治体の職員が、当社から提出された見積もりファイル\nのデータ情報（プロパティ）に、ライバル会社A社の名前が含まれているのを見つけた\nことがきっかけで発覚しました。社内でHさんに事情を聞いたところ、別の会社に\n勤める友人から公開前の見積もり情報をもらい、それを部署のチャットに投稿して、\n金額を決める参考にしたことを認めました。さらに、上司であるI課長もチャットを\n見ていながら「競争が厳しいから仕方ない」と不正を止めなかった事実も確認され、\n組織全体の管理に問題があったことが明らかになりました。",
    L"直接の原因は、営業担当Hさんの「ルールを守る」という意識が低かったことです。\nしかしその背景には、部署全体で不正を防ぐための教育がきちんと行われていな\nかったことや、成果ばかりを重視する評価制度がプレッシャーとなり、不正行為に\nつながったことがあります。この問題によって、国から罰金を科されたり、今後の\n入札に参加できなくなったりする危険性があり、会社の信用が大きく下がる恐れも\nあります。",
    L"緊急の対応として、問題となった入札はすぐに辞退し、自治体へ謝罪文を送りま\nした。社内では、Hさんを自宅待機処分とし、関わっていたI課長も管理職から外し、\n第三者委員会による調査を受けさせています。二度とこのような問題を起こさない\nため、全社員に法律に関するeラーニングを義務付け、評価項目に「ルールを守って\nいるか」という視点を加えます。また、会社の機密情報が不正にやり取りされて\nいないか監視するシステムを導入し、定期的にチェックする体制を整えました。",
    
    // 文章4：製造ライン機械故障の対応
    L"2025年2月14日の午後2時半ごろ、工場の製造ラインにある機械（WB-07）が突然\n止まり、動かなくなりました。この機械が止まる直前に作っていた製品（ロット\nB2-0214）は、品質基準を満たしていない可能性が高いため、品質部門がこのロット\nの製品6,000個をすべて詳しく検査する必要が出てきました。",
    L"機械は作業の途中でエラー表示を出して緊急停止しました。画面には「モーターを\n動かすための電源の電圧が下がっている」と表示されていました。すぐに担当者が\n駆けつけて電源部分を調べたところ、部品の劣化によって電気が不安定になり、\n本来24V必要な電圧が20Vまで落ちていたことが分かりました。さらに、機械内部の\n温度記録を確認すると、ここ2週間、決められた上限温度60℃を少し超えた状態で\n動き続けていたことも判明しました。このせいで、モーターが正常に動かず、\n製品の加工にズレが生じた可能性が高いと考えられます。",
    L"原因は、機械を冷やすための冷却ファンのフィルター掃除が、定期的なメンテナンス\n計画から漏れていたことでした。その結果、機械内部の温度が高い状態が続き、\n電源部品の寿命が縮まってしまいました。この故障の影響で、製造ラインは4時間\n48分停止し、生産計画に1日の遅れが出ています。また、問題の製品6,000個は、\nすべて品質検査が終わるまで出荷できません。この検査にかかる追加費用が約120\n万円、もし納期に間に合わなければ、お客様への違約金として最大で250万円の\n損失が出る可能性があります。",
    L"緊急の対応として、故障した電源部品を予備のものと交換し、翌朝までにラインを\n復旧させました。二度と同じ問題を起こさないために、冷却ファンフィルターの\n掃除を毎週の点検項目に加え、忘れないように管理システムに自動で通知が出る\nように設定しました。また、機械の温度が上がりすぎた場合に早めに気づけるよう、\n警告が出る温度を60℃から58℃に引き下げました。さらに、電源部品が壊れる前に\n交換する「予防交換」のルールも作り、品質トラブルが起きるリスクを減らします。",
    
    // 文章5：財務レポート
    L"このレポートは、2025年6月の会社の財務状況をまとめ、今後の経営判断に\n役立ててもらうためのものです。背景として、少し前に始めた新しいビジネスへの\n投資が、会社の利益にどう影響しているかが見えてきました。そのため、お金の\n流れ（キャッシュフロー）と利益の状況を早めに確認する必要がありました。\nまた、銀行との融資の話し合いに備えて、会社の財務が健全であることを示す\nデータも求められていました。",
    L"売上は3億2,000万円で、去年の同じ月より12%増えました。しかし、本業の利益\n（営業利益）は2,000万円の赤字でした。これは、新しいビジネスを始めるための\n広告費や人件費が増えたことが主な原因です。本業以外で補助金が500万円入った\nため、会社全体の利益（経常利益）の赤字は1,500万円となりました。お金の流れに\nついては、本業で500万円のプラス、投資で3,000万円のマイナス、借入などで\n2,000万円のプラスとなり、月末時点での現金は1億5,000万円を保っています。",
    L"売上は伸びているものの、利益が出にくくなっていることがはっきりしました。\nそのため、経営陣からは、使った費用に対してどれだけの効果があるのかを、\nもっと速く分析するように指示が出ました。この報告書から、広告にかけた費用が\nどれくらいで回収できるかを毎月チェックしていく方針が決まりました。また、\n銀行との話し合いでは、お金の流れを詳しく説明し、財務を良くしていく計画を\n示したことで、これまでの融資条件を変えずに済みました。",
    L"急いでやるべきことは、広告費の効果をきちんと測り、販売にかかる費用を\n最適化することです。そのために、マーケティング部門と協力し、どの広告が\nどれだけ効果を上げているか一目でわかるような仕組みを作ります。また、\n人件費が増えている原因であるエンジニアの採用計画を見直し、社内で対応する\n仕事と外部に委託する仕事のバランスを考え直します。さらにお金の流れを\n安定させるため、お客様からの代金をより早く回収できるような工夫（報奨金など）を\n営業部に提案します。",
    
    // 文章6：システム障害対応報告
    L"2025年4月10日の深夜、当社の主要サービス「CloudTask Manager」の動きがとても\n遅くなる問題が約50分間発生しました。この影響で、国内外の約35%のお客様が\nサービスを使えない状態になりました。日本の深夜でしたが、利用者が多い北米では\nピーク時間だったため、SNS上でお客様から「サービスが動かない」という報告が\nたくさん上がりました。そのため、原因の調査と再発防止策が急務となりました。",
    L"深夜1時14分に、システムを監視する仕組みから自動で「異常発生」の知らせが\n届き、待機していたエンジニアのGさんが対応を始めました。システムの状況を\n分析したところ、サーバー3台のうち2台がほぼ限界の状態で動いており、\nコンピューターの力が足りなくなっていることを確認しました。さらに調べると、\n深夜1時5分に自動で更新された新しいバージョン（v5.2.1）のプログラムに、\nメモリをどんどん食いつぶしてしまう不具合（メモリリーク）があったことが\n原因だと判明しました。Gさんはすぐに問題のバージョンを取り消し、元の\nバージョンに戻す作業（ロールバック）を行い、サーバーは正常な状態に戻りました。\nその後、安定して動いていることを確認し、深夜2時3分にこの問題への対応を完了しました。",
    L"直接の原因は、新しいバージョンに追加されたレポート作成機能の不具合でした。\nこの機能が、使ったメモリをきちんと解放し忘れたため、「メモリリーク」が\n発生してしまいました。この影響で、サービスが部分的に51分間止まり、\nお客様との約束（SLA）を破ってしまうことになりました。これによる違約金は\n約12万円と見られ、お客様からの問い合わせは38件にのぼりました。幸い、\nお客様のデータが消えたり、外部から攻撃されたりした形跡はありませんでした。",
    L"緊急の対応として、問題があったバージョンをすぐに元に戻しました。そして、\n不具合を修正したバージョン（v5.2.2）を、翌日までにテスト用の環境へ準備\nしました。二度と同じ問題を起こさないために、今後はプログラム開発の途中で\nメモリの不具合を自動的にチェックするツールを導入します。また、新しい\nバージョンへの更新をよりゆっくり進めることで、問題が起きてもすぐに気づける\nようにします。さらに、今回の反省会をエンジニア全員で行い、「メモリリークを\nゼロにする」ことをチームの次の目標に掲げ、品質管理を徹底していきます。",
    
    // 文章7：労働災害報告書
    L"この報告書は、2025年7月15日の午前10時半ごろ、本社工場の第3製造ラインで\n作業員の人がけがをした事故について、状況と最初の対応をまとめたものです。\n止まっていたベルトコンベアを掃除しているときに、機械が急に動き出したため、\n作業をしていたAさんの右手の甲に、打撲と切り傷ができてしまいました。幸い\n骨は折れておらず軽いけがでしたが、この事故でラインが45分間止まり、生産計画に\n遅れが出たため、管理者と安全委員会へすぐに報告する必要があります。",
    L"事故が起きたとき、製造ラインは定期点検のために止まっていました。現場の\n責任者Bさんの指示で、機械が動かないように安全ロックをかけた後、2人の作業員が\n掃除を始めていました。午前10時28分、隣のラインの責任者であるCさんが、間違えて\n共通の電源スイッチ（ブレーカー）を入れてしまったため、コンベアが再び動き出し、\n掃除をしていたAさんの手が機械に挟まれました。すぐに非常停止ボタンが押された\nため、機械は10秒以内に止まりました。社内の救護担当が5分で駆けつけて応急処置を\n行い、Aさんは救急車で近くの病院に運ばれました。",
    L"事故の原因は、複数のラインで共通になっている電源スイッチに、「使用中」で\nあることが分かるような目印がついていなかったこと、そして、作業マニュアルに\n「機械を完全に止め、動かないように札をかける」という安全ルールの説明が\n書かれていなかったことです。このせいで、隣のラインの責任者は、止まっている\nラインの電源は別だと思い込み、スイッチを入れてしまいました。この事故で\n作業員がけがをしただけでなく、ラインが45分止まったことで約30万円の生産ロスが\n出てしまいました。",
    L"けがをしたAさんは、病院で3針ぬいました。「治るまで10日くらいかかる」と、\nお医者さんに言われました。元気になるまでは、楽な仕事をしてもらいます。\n二度と事故が起きないように、電源スイッチには赤い札とカギをかける決まりを\nマニュアルに新しく書きました。止まっている機械のランプも、赤色に変えて\n分かりやすくしました。さらに、今月中に働いている人みんなで安全についての\n勉強会を開いて、しるしの確認や声かけを、みんなが必ずやるようにします。",
    
    // 文章8：ヒヤリハット報告書
    L"この報告書は、2025年6月3日の午後4時過ぎに、会社のオフィスで起きた出来事に\nついてまとめたものです。書類棚の一番上から資料のファイルが落ちてきて、\n近くを歩いていた社員Dさんの頭をかすめました。幸い、ファイルが直接ぶつかる\nことはなく、けがをした人はいませんでした。しかし、一歩間違えれば大きな事故に\nなっていた可能性があったため、「ヒヤリ」としたこの出来事をすぐにみんなで\n共有すべきだと考え、この報告書を作りました。",
    L"事故が起きる前、総務の社員が、監査で使う資料を探すために書類棚の扉を\n15分ほど繰り返し開け閉めしていました。棚の一番上には、重いファイルが3列に\n重なって置いてあり、扉の開け閉めの振動で、奥にあったファイルが少しずつ前に\nずれてきていました。ちょうどその時、外出先から戻ってきた営業のDさんが棚の前を\n通りかかり、高さ180センチの場所からファイルが落下。ファイルはDさんの肩に\n少し触れるような形で床に落ちました。大きな音を聞いた周りの社員5人が駆けつけ\nましたが、幸いけがはなかったため、Dさんはそのまま仕事に戻りました。",
    L"直接の原因は、重いファイルを棚の一番上に置いていたという、置き方のまずさ\nです。また、棚が倒れないようにするための金具が緩んでいて棚全体が少し前に\n傾いていたことや、扉を何度も開け閉めした振動が重なったことも原因です。\nこの出来事で、もう少しでけが人が出るところだったほか、周りの社員を不安な\n気持ちにさせてしまい、一時的に仕事の効率が落ちてしまいました。また、\n落ちたファイルが壊れて中の書類が散らかったため、片付けに30分ほどかかりました。",
    L"事故が起きた直後、総務の担当者は、危ないので書類棚の周りに誰も近づけない\nようにしました。今後の対策として、重い資料は胸より下の高さに移動させ、\nすべての棚で金具の緩みがないかチェックし、緩んでいた部分はすぐに締め直し\nました。また、安全な棚の管理方法についてルールを作り、毎月の安全会議で\nみんなに伝えました。さらに、「ヒヤリ」とした出来事を報告しやすくする仕組みを\n強化し、毎週メールで注意を呼びかけることで、危ないことへの意識を高めて\nいきます。"
};

static int g_currentSection = 0;
static int g_participantNumber = 1; // 参加者番号（1-8）

constexpr int kGazeMarginPx = 20; // tolerance around last character - reduced for stricter detection
constexpr int kBaseDwellTimeMs = 1000; // base gaze dwell time before advancing (in milliseconds)

// 遅延時間条件（秒）
static const std::vector<float> g_delayConditions = {0.0f, 0.3f, 0.6f, 0.9f, 1.2f, 1.5f, 1.8f, 2.1f};

// 8×32ラテン方格（各行が参加者、各列がセクション、値が遅延条件のインデックス）
// 32セクションを8つの遅延条件でカウンターバランス
static const int g_latinSquare[8][32] = {
    // 参加者1: 0,1,2,3,4,5,6,7のパターンを4回繰り返し
    {0,1,2,3,4,5,6,7, 0,1,2,3,4,5,6,7, 0,1,2,3,4,5,6,7, 0,1,2,3,4,5,6,7},
    // 参加者2: 1つずつシフト
    {1,2,3,4,5,6,7,0, 1,2,3,4,5,6,7,0, 1,2,3,4,5,6,7,0, 1,2,3,4,5,6,7,0},
    // 参加者3: 2つずつシフト
    {2,3,4,5,6,7,0,1, 2,3,4,5,6,7,0,1, 2,3,4,5,6,7,0,1, 2,3,4,5,6,7,0,1},
    // 参加者4: 3つずつシフト
    {3,4,5,6,7,0,1,2, 3,4,5,6,7,0,1,2, 3,4,5,6,7,0,1,2, 3,4,5,6,7,0,1,2},
    // 参加者5: 4つずつシフト
    {4,5,6,7,0,1,2,3, 4,5,6,7,0,1,2,3, 4,5,6,7,0,1,2,3, 4,5,6,7,0,1,2,3},
    // 参加者6: 5つずつシフト
    {5,6,7,0,1,2,3,4, 5,6,7,0,1,2,3,4, 5,6,7,0,1,2,3,4, 5,6,7,0,1,2,3,4},
    // 参加者7: 6つずつシフト
    {6,7,0,1,2,3,4,5, 6,7,0,1,2,3,4,5, 6,7,0,1,2,3,4,5, 6,7,0,1,2,3,4,5},
    // 参加者8: 7つずつシフト
    {7,0,1,2,3,4,5,6, 7,0,1,2,3,4,5,6, 7,0,1,2,3,4,5,6, 7,0,1,2,3,4,5,6}
};

// 現在のセクションの遅延時間を取得
float getCurrentDelayTime() {
    int delayIndex = g_latinSquare[(g_participantNumber - 1) % 8][g_currentSection % 32];
    return g_delayConditions[delayIndex];
}

// 遅延時間を含む実際の待機時間を計算
int getDwellTimeMs() {
    float delayTime = getCurrentDelayTime();
    return kBaseDwellTimeMs + static_cast<int>(delayTime * 1000);
}

static std::chrono::steady_clock::time_point g_gazeStart;
static bool g_inTarget = false;
static bool g_showGaze = true; // toggle visualization - enabled by default for debugging
static bool g_seenFirst = false; // whether the first char has been gazed at in current section
static bool g_sectionCompleted = false; // whether current section is completed and waiting for button click
static RECT g_nextButtonRect = {0, 0, 0, 0}; // button rectangle for click detection

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));

        HFONT hFont = CreateFontW(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        HFONT old = (HFONT)SelectObject(hdc, hFont);

        if (g_sectionCompleted) {
            // Show completion message and next button
            std::wstring completionMsg = L"セクション " + std::to_wstring(g_currentSection + 1) + L" 完了";
            SIZE msgSz;
            GetTextExtentPoint32W(hdc, completionMsg.c_str(), (int)completionMsg.size(), &msgSz);
            int msgX = (rc.right - msgSz.cx) / 2;
            int msgY = rc.bottom / 2 - 100;
            TextOutW(hdc, msgX, msgY, completionMsg.c_str(), (int)completionMsg.size());
            
            // Draw next button
            int buttonWidth = 200;
            int buttonHeight = 60;
            int buttonX = (rc.right - buttonWidth) / 2;
            int buttonY = rc.bottom / 2 - 10;
            
            g_nextButtonRect = {buttonX, buttonY, buttonX + buttonWidth, buttonY + buttonHeight};
            
            // Button background
            HBRUSH buttonBrush = CreateSolidBrush(RGB(70, 130, 180));
            FillRect(hdc, &g_nextButtonRect, buttonBrush);
            DeleteObject(buttonBrush);
            
            // Button border
            HPEN buttonPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
            HPEN oldPen = (HPEN)SelectObject(hdc, buttonPen);
            Rectangle(hdc, g_nextButtonRect.left, g_nextButtonRect.top, g_nextButtonRect.right, g_nextButtonRect.bottom);
            SelectObject(hdc, oldPen);
            DeleteObject(buttonPen);
            
            // Button text
            SetTextColor(hdc, RGB(255, 255, 255));
            std::wstring buttonText = (g_currentSection < (int)g_sections.size() - 1) ? L"次のセクションへ" : L"実験終了";
            SIZE btnTextSz;
            GetTextExtentPoint32W(hdc, buttonText.c_str(), (int)buttonText.size(), &btnTextSz);
            int textX = buttonX + (buttonWidth - btnTextSz.cx) / 2;
            int textY = buttonY + (buttonHeight - btnTextSz.cy) / 2;
            TextOutW(hdc, textX, textY, buttonText.c_str(), (int)buttonText.size());
            SetTextColor(hdc, RGB(0, 0, 0)); // Reset text color
        } else {
            // Draw current section centered
            std::wstring text = g_sections[g_currentSection];
            
            // Split text by newlines and draw multiple lines
            std::vector<std::wstring> lines;
            std::wstring currentLine;
            for (wchar_t c : text) {
                if (c == L'\n') {
                    lines.push_back(currentLine);
                    currentLine.clear();
                } else {
                    currentLine += c;
                }
            }
            if (!currentLine.empty()) {
                lines.push_back(currentLine);
            }

            // Calculate total height and draw centered
            int lineHeight = 80; // Line spacing - increased further for better readability
            int totalHeight = (int)lines.size() * lineHeight;
            int startY = (rc.bottom - totalHeight) / 2;
            
            for (size_t i = 0; i < lines.size(); ++i) {
                SIZE sz;
                GetTextExtentPoint32W(hdc, lines[i].c_str(), (int)lines[i].size(), &sz);
                int x = (rc.right - sz.cx) / 2;
                int y = startY + (int)i * lineHeight;
                TextOutW(hdc, x, y, lines[i].c_str(), (int)lines[i].size());
            }
        }

        // Display section info and participant info in top-right corner (only if not completed)
        if (!g_sectionCompleted) {
            HFONT hSmallFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                           DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            SelectObject(hdc, hSmallFont);
            
            // セクション情報
            std::wstring info = L"統合版 - " + std::to_wstring(g_currentSection + 1) + L"/" + std::to_wstring(g_sections.size());
            SIZE infoSz;
            GetTextExtentPoint32W(hdc, info.c_str(), (int)info.size(), &infoSz);
            TextOutW(hdc, rc.right - infoSz.cx - 10, 10, info.c_str(), (int)info.size());
            
            // 参加者情報
            std::wstring participantInfo = L"参加者: " + std::to_wstring(g_participantNumber) + L"/8";
            SIZE participantSz;
            GetTextExtentPoint32W(hdc, participantInfo.c_str(), (int)participantInfo.size(), &participantSz);
            TextOutW(hdc, rc.right - participantSz.cx - 10, 30, participantInfo.c_str(), (int)participantInfo.size());
            
            // 現在の遅延時間
            float currentDelay = getCurrentDelayTime();
            std::wstring delayInfo = L"遅延: " + std::to_wstring(currentDelay) + L"秒";
            SIZE delaySz;
            GetTextExtentPoint32W(hdc, delayInfo.c_str(), (int)delayInfo.size(), &delaySz);
            TextOutW(hdc, rc.right - delaySz.cx - 10, 50, delayInfo.c_str(), (int)delayInfo.size());
            
            DeleteObject(hSmallFont);
        }

        // Display control instructions in top-left corner
        HFONT hInstructionFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                            DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        SelectObject(hdc, hInstructionFont);
        
        std::wstring instructions[] = {
            L"操作方法:",
            L"1-8: 参加者番号変更",
            L"↑↓: 参加者番号調整",
            L"G: 視線表示切替",
            L"クリック: 次へ進む"
        };
        
        for (int i = 0; i < 5; i++) {
            TextOutW(hdc, 10, 10 + i * 15, instructions[i].c_str(), (int)instructions[i].size());
        }
        
        DeleteObject(hInstructionFont);

        SelectObject(hdc, old);
        DeleteObject(hFont);

        // Draw gaze point if visualization enabled
        if (g_showGaze)
        {
            float gx = g_gazeX.load();
            float gy = g_gazeY.load();
            if (gx >= 0 && gy >= 0)
            {
                int px = (int)(gx * (rc.right - rc.left));
                int py = (int)(gy * (rc.bottom - rc.top));

                // red filled circle
                HBRUSH hDot = CreateSolidBrush(RGB(255, 0, 0));
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, hDot);
                HPEN oldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
                int r = 6;
                Ellipse(hdc, px - r, py - r, px + r, py + r);
                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                DeleteObject(hDot);

                // show numeric coordinates
                std::wstring coord = L"(" + std::to_wstring(px) + L", " + std::to_wstring(py) + L")";
                TextOutW(hdc, 10, 70, coord.c_str(), (int)coord.size());
            }
        }

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_KEYDOWN:
        if (wParam == 'G')
        {
            g_showGaze = !g_showGaze;
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        else if (wParam >= '1' && wParam <= '8')
        {
            // 参加者番号を変更（1-8）
            g_participantNumber = wParam - '0';
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        else if (wParam == VK_UP)
        {
            // 参加者番号を増加
            g_participantNumber = (g_participantNumber % 8) + 1;
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        else if (wParam == VK_DOWN)
        {
            // 参加者番号を減少
            g_participantNumber = ((g_participantNumber - 2 + 8) % 8) + 1;
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;
    case WM_LBUTTONDOWN:
        if (g_sectionCompleted) {
            // Check if click is within button area
            int mouseX = LOWORD(lParam);
            int mouseY = HIWORD(lParam);
            
            if (mouseX >= g_nextButtonRect.left && mouseX <= g_nextButtonRect.right &&
                mouseY >= g_nextButtonRect.top && mouseY <= g_nextButtonRect.bottom) {
                
                // Button clicked - advance to next section or end experiment
                if (g_currentSection < (int)g_sections.size() - 1) {
                    g_currentSection++;
                    g_sectionCompleted = false;
                    g_seenFirst = false;
                    g_inTarget = false;
                    InvalidateRect(hWnd, nullptr, FALSE);
                } else {
                    // All sections completed
                    MessageBox(hWnd, _T("統合版レポート集完了！"), _T("完了"), MB_OK);
                    PostQuitMessage(0);
                }
            }
        }
        return 0;
    case WM_TIMER:
        // Reduce flickering by limiting redraws - only update gaze visualization when needed
        if (g_showGaze && g_gazeUpdated.exchange(false)) {
            // Only invalidate the gaze area instead of the whole window
            InvalidateRect(hWnd, nullptr, FALSE);
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void gaze_callback(TobiiResearchGazeData* gaze, void*)
{
    const TobiiResearchGazePoint& lgp = gaze->left_eye.gaze_point;
    const TobiiResearchGazePoint& rgp = gaze->right_eye.gaze_point;
    float x = -1, y = -1;
    if (lgp.validity == TOBII_RESEARCH_VALIDITY_VALID) { x = lgp.position_on_display_area.x; y = lgp.position_on_display_area.y; }
    else if (rgp.validity == TOBII_RESEARCH_VALIDITY_VALID) { x = rgp.position_on_display_area.x; y = rgp.position_on_display_area.y; }
    if (x >= 0 && y >= 0) {
        g_gazeX.store(x);
        g_gazeY.store(y);
        g_gazeUpdated.store(true);
    }
}

bool isInsideLastChar(HWND hWnd)
{
    float gx = g_gazeX.load();
    float gy = g_gazeY.load();
    if (gx < 0) return false;

    RECT rc; GetClientRect(hWnd, &rc);
    int screenW = rc.right - rc.left;
    int screenH = rc.bottom - rc.top;
    int px = (int)(gx * screenW);
    int py = (int)(gy * screenH);

    HDC hdc = GetDC(hWnd);
    std::wstring text = g_sections[g_currentSection];
    HFONT hFont = CreateFontW(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT old = (HFONT)SelectObject(hdc, hFont);
    
    // Split text by newlines
    std::vector<std::wstring> lines;
    std::wstring currentLine;
    for (wchar_t c : text) {
        if (c == L'\n') {
            lines.push_back(currentLine);
            currentLine.clear();
        } else {
            currentLine += c;
        }
    }
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    RECT last{};
    if (!lines.empty()) {
        std::wstring lastLine = lines.back();
        if (!lastLine.empty()) {
            SIZE szLastLine, szLastChar;
            GetTextExtentPoint32W(hdc, lastLine.c_str(), (int)lastLine.size(), &szLastLine);
            GetTextExtentPoint32W(hdc, lastLine.c_str() + lastLine.size() - 1, 1, &szLastChar);
            
            int lineHeight = 80;
            int totalHeight = (int)lines.size() * lineHeight;
            int startY = (rc.bottom - totalHeight) / 2;
            int lastX = (rc.right - szLastLine.cx) / 2;
            int lastY = startY + ((int)lines.size() - 1) * lineHeight;
            
            // Create a more generous bounding box for the last character
            last = { lastX + szLastLine.cx - szLastChar.cx - 10, lastY - 5, 
                    lastX + szLastLine.cx + 10, lastY + 37 };
        }
    }
    
    SelectObject(hdc, old);
    DeleteObject(hFont);
    ReleaseDC(hWnd, hdc);

    InflateRect(&last, kGazeMarginPx, kGazeMarginPx);
    return (px >= last.left && px <= last.right && py >= last.top && py <= last.bottom);
}

bool isInsideFirstChar(HWND hWnd)
{
    float gx = g_gazeX.load();
    float gy = g_gazeY.load();
    if (gx < 0) return false;

    RECT rc; GetClientRect(hWnd, &rc);
    int screenW = rc.right - rc.left;
    int screenH = rc.bottom - rc.top;
    int px = (int)(gx * screenW);
    int py = (int)(gy * screenH);

    HDC hdc = GetDC(hWnd);
    std::wstring text = g_sections[g_currentSection];
    HFONT hFont = CreateFontW(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT old = (HFONT)SelectObject(hdc, hFont);
    
    // Split text by newlines
    std::vector<std::wstring> lines;
    std::wstring currentLine;
    for (wchar_t c : text) {
        if (c == L'\n') {
            lines.push_back(currentLine);
            currentLine.clear();
        } else {
            currentLine += c;
        }
    }
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }
    
    RECT first{};
    if (!lines.empty()) {
        std::wstring firstLine = lines[0];
        SIZE szFirstLine, szFirstChar;
        GetTextExtentPoint32W(hdc, firstLine.c_str(), (int)firstLine.size(), &szFirstLine);
        GetTextExtentPoint32W(hdc, firstLine.c_str(), 1, &szFirstChar);
        
        int lineHeight = 80;
        int totalHeight = (int)lines.size() * lineHeight;
        int startY = (rc.bottom - totalHeight) / 2;
        int firstX = (rc.right - szFirstLine.cx) / 2;
        first = { firstX, startY, firstX + szFirstChar.cx, startY + 32 };
    }
    
    SelectObject(hdc, old);
    DeleteObject(hFont);
    ReleaseDC(hWnd, hdc);

    InflateRect(&first, kGazeMarginPx, kGazeMarginPx);
    return (px >= first.left && px <= first.right && py >= first.top && py <= first.bottom);
}

int APIENTRY _tWinMain(HINSTANCE hInst, HINSTANCE, LPTSTR, int)
{
    const TCHAR CLASS_NAME[] = _T("TobiiSeqWnd");
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    RECT work{}; SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);
    int sw = work.right - work.left;
    int sh = work.bottom - work.top;

    HWND hWnd = CreateWindowEx(0, CLASS_NAME, _T("統合版レポート集"), WS_POPUP | WS_VISIBLE,
        work.left, work.top, sw, sh, nullptr, nullptr, hInst, nullptr);
    if (!hWnd) return 0;

    SetTimer(hWnd, 1, 16, nullptr); // refresh timer

    // Tobii Init
    TobiiResearchEyeTrackers* trackers = nullptr;
    if (tobii_research_find_all_eyetrackers(&trackers) != TOBII_RESEARCH_STATUS_OK || trackers->count == 0) {
        MessageBox(nullptr, _T("Eye tracker not found"), _T("Error"), MB_ICONERROR);
        return 0;
    }
    TobiiResearchEyeTracker* tracker = trackers->eyetrackers[0];
    tobii_research_subscribe_to_gaze_data(tracker, gaze_callback, nullptr);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        bool firstInside = isInsideFirstChar(hWnd);
        if (firstInside)
        {
            g_seenFirst = true; // gate has been met
        }

        // Only process gaze tracking if section is not completed
        if (!g_sectionCompleted) {
            bool inside = isInsideLastChar(hWnd);
            auto now = std::chrono::steady_clock::now();
            if (g_seenFirst && inside) {
                if (!g_inTarget) {
                    g_inTarget = true;
                    g_gazeStart = now;
                    // No need to invalidate here - text doesn't change
                }
            }

            // Check timer if we've started the dwell process
            if (g_inTarget) {
                auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_gazeStart);
                int currentDwellTime = getDwellTimeMs();
                if (dur.count() > currentDwellTime) {
                    // Section completed - show button
                    g_sectionCompleted = true;
                    g_inTarget = false;
                    InvalidateRect(hWnd, nullptr, FALSE); // force redraw to show button
                }
            }
        }
    }

    tobii_research_unsubscribe_from_gaze_data(tracker, gaze_callback);
    tobii_research_free_eyetrackers(trackers);
    return 0;
}
