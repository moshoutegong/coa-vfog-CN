#include "settings_panel.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <string_view>

namespace
{
constexpr float kMultiplierSliderMax = 4.0f;
constexpr float kPanelWidthInLines = 30.0f;
constexpr float kPanelHeightInLines = 42.0f;
constexpr float kMinPanelWidthInLines = 20.0f;
constexpr float kMinPanelHeightInLines = 16.0f;
constexpr float kPanelMarginInLines = 2.0f;
constexpr float kLabelWidthInLines = 11.0f;
constexpr float kLabelShareOfWidth = 0.45f;
constexpr float kFirstFooterLines = 4.0f;
constexpr float kNoTextWrap = -1.0f;
constexpr size_t kStatusTextSize = 512;
const ImVec4 kDrawnColour = {0.45f, 0.85f, 0.45f, 1.0f};
const ImVec4 kSkippedColour = {0.95f, 0.75f, 0.35f, 1.0f};
const ImVec4 kErrorColour = {1.0f, 0.4f, 0.4f, 1.0f};

const char* const kQualityNames[] = {"低：四分之一分辨率，16 步", "中：半分辨率，24 步", "高：半分辨率，32 步"};
const char* const kDebugViewNames[] = {"关闭", "雾光亮度", "透射率", "线性深度"};
const char* const kLogLevelNames[] = {"错误", "信息", "调试"};
const char* const kWaterQualityNames[] = {"低：128 网格，仅天空反射", "中：256 网格，场景反射",
                                         "高：256 网格，精细场景反射"};
const char* const kWaterDebugViewNames[] = {"关闭", "法线", "泡沫", "透射率", "反射", "液体类别", "涟漪"};

void HelpTooltip(const char* help)
{
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
        ImGui::TextUnformatted(help);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

template <int N>
bool Choice(const char* label, int& value, int first, const char* const (&names)[N], const char* help)
{
    int index = value - first;
    const bool changed = ImGui::Combo(label, &index, names, N);
    HelpTooltip(help);
    if (changed)
        value = index + first;
    return changed;
}

class Section
{
public:
    explicit Section(const char* title, ImGuiTreeNodeFlags flags = 0)
        : m_title(title), m_open(ImGui::CollapsingHeader(title, flags))
    {
        if (m_open)
            ImGui::PushID(m_title);
    }
    ~Section()
    {
        if (m_open)
            ImGui::PopID();
    }
    Section(const Section&) = delete;
    Section& operator=(const Section&) = delete;
    explicit operator bool() const { return m_open; }

private:
    const char* m_title;
    bool m_open;
};

bool Slider(const char* label, float& value, float lo, float hi, const char* format, const char* help,
            ImGuiSliderFlags flags = ImGuiSliderFlags_None)
{
    const bool changed = ImGui::SliderFloat(label, &value, lo, hi, format, flags);
    HelpTooltip(help);
    return changed;
}

bool Multiplier(const char* label, float& value, const char* help)
{
    return Slider(label, value, 0.0f, kMultiplierSliderMax, "%.2f", help);
}

bool Toggle(const char* label, bool& value, const char* help)
{
    const bool changed = ImGui::Checkbox(label, &value);
    HelpTooltip(help);
    return changed;
}

bool Toggle(const char* label, int& value, const char* help)
{
    bool on = value != 0;
    const bool changed = Toggle(label, on, help);
    if (changed)
        value = on ? 1 : 0;
    return changed;
}

struct StatusTranslation
{
    std::string_view english;
    const char* chinese;
};

constexpr StatusTranslation kStatusTranslations[] = {
    {"waiting for the world to render", "等待场景渲染"},
    {"stopped after an exception, see CoAVolFog.log", "发生异常后停止，详见 CoAVolFog.log"},
    {"camera under liquid", "镜头位于液面下"},
    {"the world render inputs were not captured", "未获取场景渲染数据"},
    {"invalid frame inputs", "帧数据无效"},
    {"camera under water", "镜头位于水下"},
    {"the client's water colours are unavailable", "无法获取客户端水色"},
    {"the water pass could not start", "无法启动水面渲染"},
    {"the fog stopped after an exception", "雾效发生异常后停止"},
    {"no water in view", "视野内没有水面"},
    {"no fog device", "雾效渲染设备不可用"},
    {"the water pass shaded nothing", "没有渲染到水面"},
    {"the water pass did not finish", "水面渲染未完成"},
    {"no liquid pass in this frame", "本帧没有液体渲染"},
    {"hooks not installed", "渲染挂钩未安装"},
    {"off", "已关闭"},
    {"no render target", "缺少渲染目标"},
    {"fog depth surface not bound", "雾效深度表面未绑定"},
    {"surface description failed", "无法读取表面信息"},
    {"render target and depth sizes differ", "渲染目标和深度表面尺寸不一致"},
    {"render target and depth sample counts differ", "渲染目标和深度表面采样数不一致"},
    {"world viewport outside the render target", "场景视口超出渲染目标"},
    {"required shader unsupported", "不支持所需的着色器"},
    {"shader creation failed", "无法创建着色器"},
    {"state block recording failed", "无法记录渲染状态块"},
    {"render target creation failed", "无法创建渲染目标"},
    {"device not ready", "渲染设备尚未就绪"},
    {"god ray targets unavailable", "阳光光束渲染目标不可用"},
    {"no scene copy for the god rays", "阳光光束所需的场景副本不可用"},
    {"view matrix not invertible", "视图矩阵不可逆"},
    {"multisampled depth copy failed", "无法复制多重采样深度"},
    {"water shader unsupported", "不支持水面着色器"},
    {"water shader creation failed", "无法创建水面着色器"},
    {"water state block recording failed", "无法记录水面渲染状态块"},
    {"water copies unavailable", "水面渲染所需的副本不可用"},
    {"water copy creation failed", "无法创建水面渲染所需的副本"},
    {"flat water texture creation failed", "无法创建平静水面纹理"},
    {"water disabled", "水面效果已关闭"},
    {"no water data", "缺少水面数据"},
    {"fog depth unavailable", "雾效深度不可用"},
    {"scene colour copy failed", "无法复制场景颜色"},
    {"water pass not armed", "水面渲染尚未准备好"},
    {"no water drawn", "没有渲染到水面"},
    {"no water preset for the drawn classes", "当前水域没有对应的预设"},
    {"the game's Multisampling option is 1x", "游戏的多重采样设置为 1 倍"},
    {"Multisampling=0 in CoAVolFog.ini", "CoAVolFog.ini 中的 Multisampling 为 0"},
    {"the depth copy self-test failed", "深度复制自检失败"},
    {"the game's depth buffer is unavailable", "游戏的深度缓冲不可用"},
    {"the INTZ copy could not be created", "无法创建 INTZ 深度副本"},
    {"the depth copy could not start", "无法启动深度复制"},
    {"the device could not be created with the game's multisampling", "无法使用游戏的多重采样设置创建渲染设备"},
    {"Reset with the game's multisampling failed", "无法使用游戏的多重采样设置重置渲染设备"},
    {"the game's depth format has no stencil and D24S8 is not available with its multisampling",
     "游戏深度格式缺少模板缓冲，当前多重采样也不支持 D24S8"},
    {"the driver has no RESZ depth resolve and the adapter is not NVIDIA", "驱动不支持 RESZ 深度解析，且显卡非 NVIDIA"},
    {"d3d9.dll is not the system copy (DXVK or another wrapper), so NVAPI cannot copy its depth",
     "当前 d3d9.dll 由 DXVK 等封装层提供，NVAPI 无法复制深度"},
    {"nvapi.dll is missing from the system folder", "系统目录中缺少 nvapi.dll"},
    {"NvAPI_Initialize failed", "NVAPI 初始化失败"},
    {"this NVAPI has no D3D9 depth copy", "此 NVAPI 版本不支持 D3D9 深度复制"},
    {"no depth copy method (forced by the harness)", "测试程序强制关闭深度复制"},
    {"lake", "湖泊"},
    {"river", "河流"},
    {"ocean", "海洋"},
    {"interior", "室内水域"},
    {"flat waves", "平静水面"},
};

std::string_view TranslateStatus(std::string_view reason)
{
    for (const auto& entry : kStatusTranslations)
        if (entry.english == reason)
            return entry.chinese;
    return reason;
}

std::string TranslateWaterClasses(std::string_view classes)
{
    std::string translated;
    while (!classes.empty())
    {
        const auto separator = classes.find_first_of(",;");
        translated += TranslateStatus(classes.substr(0, separator));
        if (separator == std::string_view::npos)
            break;
        translated += classes[separator] == ',' ? "、" : "；";
        classes.remove_prefix(separator + 1);
        const auto next = classes.find_first_not_of(' ');
        if (next == std::string_view::npos)
            break;
        classes.remove_prefix(next);
    }
    return translated;
}

void DrawStatus(const FogFrameStatus& status)
{
    const std::string_view reason = TranslateStatus(status.reason ? status.reason : "原因未知");
    if (status.drawn)
    {
        ImGui::TextColored(kDrawnColour, "体积雾：正在渲染");
        if (!reason.empty())
            ImGui::TextWrapped("%.*s", static_cast<int>(reason.size()), reason.data());
    }
    else
        ImGui::TextColored(kSkippedColour, "体积雾：未渲染（%.*s）", static_cast<int>(reason.size()), reason.data());
}

void StatusLine(const ImVec4& colour, const char* format, ...)
{
    char text[kStatusTextSize];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    ImGui::PushTextWrapPos(kNoTextWrap);
    ImGui::TextColored(colour, "%s", text);
    ImGui::PopTextWrapPos();
    HelpTooltip(text);
}

void DrawWaterStatus(const WaterFrameStatus& status)
{
    const bool named = status.reason && *status.reason;
    if (status.drawn && named)
        StatusLine(kDrawnColour, "水面：正在渲染（%s）", TranslateWaterClasses(status.reason).c_str());
    else if (status.drawn)
        StatusLine(kDrawnColour, "水面：正在渲染");
    else
    {
        const auto reason = TranslateStatus(named ? status.reason : "原因未知");
        StatusLine(kSkippedColour, "水面：未渲染（%.*s）", static_cast<int>(reason.size()), reason.data());
    }
}

void DrawMultisamplingStatus(const MultisamplingStatus& status)
{
    if (status.method && *status.method)
        StatusLine(kDrawnColour, "抗锯齿：保留 %d 倍多重采样（通过 %s 复制深度）", status.samples,
                   status.method);
    else
    {
        const auto reason = TranslateStatus(status.off ? status.off : "原因未知");
        StatusLine(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "抗锯齿：多重采样已关闭（%.*s）",
                   static_cast<int>(reason.size()), reason.data());
    }
}

bool DrawQuality(Config& c)
{
    const Section section("画质", ImGuiTreeNodeFlags_DefaultOpen);
    if (!section)
        return false;
    bool changed = Choice("画质", c.quality, 1, kQualityNames, "设置体积雾渲染的分辨率和光线步进次数。");
    changed |= Slider("时域滤波", c.temporal, 0.0f, 0.97f, "%.2f",
                      "上一帧在时域滤波中的权重。0 为关闭。");
    return changed;
}

bool DrawDensity(Config& c)
{
    const Section section("雾层与密度", ImGuiTreeNodeFlags_DefaultOpen);
    if (!section)
        return false;
    bool changed = Multiplier("总体密度", c.density,
                              "雾的浓度会影响能见度和散射光。日月周围的光晕可用「日月散射」调节。");
    changed |= Multiplier("薄雾", c.haze, "缺少经典版数据时使用的远处薄雾。");
    changed |= Multiplier("贴地雾", c.groundFog, "缺少经典版数据时使用的近地雾层。");
    changed |= Multiplier("远景雾", c.farFog, "控制用于替代原版雾效的远景雾浓度。");
    changed |= Toggle("替换原版雾效", c.stockFog,
                      "开启后用体积远景雾替换客户端的线性雾。关闭后保留原版雾效。");
    changed |= Toggle("经典版雾层", c.dataMode,
                      "在经典版光照数据覆盖的区域使用其雾层。关闭后，各区域均使用推算出的雾层。");
    changed |= Slider("雾效范围", c.maxDistance, 200.0f, 5000.0f, "%.0f 码",
                      "控制天空方向的雾效积分距离，以及经典版距离衰减曲线的尺度。");
    changed |= Slider("密度变化", c.noiseAmount, 0.0f, 1.0f, "%.2f",
                      "控制薄雾和贴地雾的空间密度变化。0 时保持原设定的平滑密度。");
    changed |= Slider("变化尺度", c.noiseScale, 0.001f, 1.0f, "%.3f",
                      "数值越大，雾团越小。", ImGuiSliderFlags_Logarithmic);
    changed |= Slider("雾团漂移", c.noiseWindSpeed, 0.0f, 10.0f, "%.2f 码/秒",
                      "雾团的漂移速度。0 时保持静止。");
    changed |= Toggle("经典版雾层噪声", c.classicNoise,
                      "经典版雾层带有新版噪声数据时会出现飘动的雾团，多见于风暴，平均密度约为原来的一半。关闭后改用上方的密度变化设置。");
    return changed;
}

bool DrawLight(Config& c)
{
    const Section section("光照", ImGuiTreeNodeFlags_DefaultOpen);
    if (!section)
        return false;
    bool changed = Multiplier("日月散射", c.sunScatter, "太阳或月亮的光在雾中散射，形成光晕。");
    changed |= Multiplier("环境亮度", c.ambient, "雾在环境光下的亮度。");
    changed |= Multiplier("曝光", c.exposure, "缺少经典版数据时使用的雾层亮度。");
    changed |= Multiplier("经典版雾层曝光", c.classicExposure, "经典版雾层的亮度。1 为默认值。");
    changed |= Toggle("经典版散射能量归一化", c.classicPhase,
                      "用能量归一化的相位函数计算日月光在经典版雾层中的散射。地平线的雾带会更暗，日晕更大、更亮。关闭时，朝向光源的相位函数峰值为 1。");
    changed |= Toggle("线性光照", c.colorSpace,
                      "在线性空间中计算散射和混合，使高光平滑过渡。关闭后使用伽马空间。");
    changed |= Toggle("局部光源", c.localLights, "让场景中的附近点光源在雾里散射。");
    changed |= Slider("局部光源强度", c.localLightIntensity, 0.0f, 8.0f, "%.2f",
                      "附近点光源在雾中的亮度。");
    changed |= Slider("局部光源散射方向", c.localLightPhase, -0.9f, 0.9f, "%.2f",
                      "0 时向各方向均匀散射。接近 0.9 时光源与镜头之间的雾更亮。低于 0 时光源后方的雾更亮，接近 -0.9 "
                      "时尤其明显。各点光源共用此值，0.3 为校准值。");
    changed |= Slider("阳光光束", c.godRays, 0.0f, 4.0f, "%.2f",
                      "从太阳周围明亮天空向外辐射的光束。0 为关闭。");
    changed |= Toggle("辉光补偿", c.glowCompensation,
                      "预先补偿客户端的全屏辉光，避免太阳附近的亮雾泛白。");
    return changed;
}

bool DrawGlowAndGrading(Config& c)
{
    const Section section("辉光与调色");
    if (!section)
        return false;
    bool changed = Toggle("Forever 辉光", c.foreverGlow,
                          "在经典版光照覆盖区使用新版辉光强度，多数大陆光照的值为 0。它会替换客户端辉光（含 ambientGlow），"
                      "并在覆盖区边缘渐变回客户端辉光。关闭后使用客户端辉光。");
    changed |= Slider("色彩分级", c.colorGrading, 0.0f, 1.0f, "%.2f",
                      "调整镜头附近的新版色彩曲线强度，提亮中间调，并把最高光截为白色。角色名字、界面、灵魂视角和水下画面不参与调色。0 为关闭。");
    return changed;
}

bool DrawViewDistance(Config& c)
{
    bool lifted = c.farClipMax > kFarClipMaxKeepsClientCap;
    bool changed = Toggle("提高大陆视距上限", lifted,
                          "Ascension 的大陆视距上限为 791 码，引擎最高支持 1583 码。加载的地形约为原来的四倍。如果启"
                      "动时未开启，打开后需要重启客户端。");
    if (changed)
        c.farClipMax = lifted ? kEngineFarClipMax : kFarClipMaxKeepsClientCap;
    if (lifted)
        changed |= Slider("视距", c.farClipMax, std::ceil(kEngineFarClipMin), kEngineFarClipMax, "%.0f 码",
                          "大陆视距受客户端 farclip 设置限制。",
                          ImGuiSliderFlags_AlwaysClamp);
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    ImGui::TextWrapped("下次调整 farclip、加载地图或切换区域时生效。");
    ImGui::PopStyleColor();
    return changed;
}

bool DrawWorld(Config& c)
{
    const Section section("场景");
    if (!section)
        return false;
    bool changed = Toggle("水下雾效", c.underwater, "镜头进入水下时继续渲染体积雾。");
    changed |= Toggle("按距离渲染半透明特效雾", c.transparentFog,
                      "水面之后、半透明特效之前绘制雾。粒子、法术等半透明物体按自身距离使用线性雾，在 100 码内拟合体积雾，超过后"
                      "继续延伸。远处特效可能比周围场景更浓或更淡，超过雾效终点的辉光会消失。关闭后，场景绘制完再统一绘雾，半透明特效"
                      "继承背景的雾效。");
    changed |= Toggle("水面写入深度", c.liquidDepth,
                      "使水面按自身距离计算雾效。渲染新版水面时始终写入深度。");
    changed |= Toggle("室内雾效过渡", c.interiorAware,
                      "镜头进入建筑或洞穴时，逐渐减弱室外雾和直射阳光。");
    changed |= Slider("室内雾密度", c.interiorDensity, 0.0f, 1.0f, "%.2f",
                      "室内保留的室外雾密度比例。");
    changed |= Slider("室内远景雾", c.interiorFarFog, 0.0f, 1.0f, "%.2f",
                      "室内保留的原版远景雾密度比例。室外远景雾独立计算。");
    changed |= DrawViewDistance(c);
    return changed;
}

bool DrawAntialiasing(Config& c)
{
    const Section section("抗锯齿");
    if (!section)
        return false;
    return Toggle("保留游戏多重采样", c.multisampling,
                  "显卡驱动能为雾和水面复制多重采样深度时，保留游戏的多重采样抗锯齿。NVIDIA 使用 NVAPI，AMD 和 "
                      "Intel 使用 RESZ。开启后需重启游戏，关闭后在下次重置显示时生效，例如修改分辨率或多重采样选项。");
}

bool DrawWater(Config& c)
{
    const Section section("水面");
    if (!section)
        return false;
    bool changed = Toggle("新版水面", c.water,
                          "使用新版效果渲染湖泊、河流、海洋和室内水池，同时让水面写入深度。关闭后使用客户端水面。");
    changed |= Choice("水面画质", c.waterQuality, 1, kWaterQualityNames,
                      "控制波浪模拟的网格尺寸和反射精细度。");
    changed |= Slider("波浪", c.waterWaves, 0.0f, 2.0f, "%.2f", "波浪高度。0 为平静水面。");
    changed |= Slider("风力", c.waterWind, 0.5f, 10.0f, "%.2f",
                      "驱动波浪的风力。风越强，波长越长，水面起伏越大。");
    changed |= Slider("泡沫", c.waterFoam, 0.0f, 2.0f, "%.2f", "浪尖和岸边的泡沫。0 为关闭。");
    changed |= Slider("反射", c.waterReflections, 0.0f, 2.0f, "%.2f",
                      "天空和场景的反射强度。0 为关闭。");
    changed |= Slider("日月高光", c.waterSpecular, 0.0f, 4.0f, "%.2f",
                      "太阳或月亮在水面上的高光。0 为关闭。");
    changed |= Slider("清澈度", c.waterClarity, 0.25f, 4.0f, "%.2f",
                      "影响水下可见深度。数值越大，水越清澈。", ImGuiSliderFlags_Logarithmic);
    changed |= Slider("区域水色", c.waterZoneColors, 0.0f, 1.0f, "%.2f",
                      "控制区域光照对水面颜色的影响。0 时只用新版水色。");
    changed |= Slider("涟漪", c.waterRipples, 0.0f, 2.0f, "%.2f",
                      "角色、生物、宠物和坐骑在水中奔跑或游动时会留下 V 形尾流，起步或停下时产生一圈涟漪，静止时水面趋于平静。默认"
                      " 0.5，0 为关闭。");
    changed |= Toggle("原版水花", c.waterClientSplashes,
                      "生成涟漪时保留原版水花和尾流贴图。关闭后，所有单位的原版水花和尾流都会隐藏，包括涟漪模拟范围外的单位。");
    changed |= Choice("水面调试视图", c.waterDebugView, 0, kWaterDebugViewNames,
                      "查看水面着色的指定输入数据。");
    return changed;
}

bool DrawDebug(Config& c)
{
    const Section section("调试");
    if (!section)
        return false;
    bool changed = Choice("雾效调试视图", c.debugView, 0, kDebugViewNames, "查看雾效渲染的指定输入数据。");
    changed |= Toggle("太阳位置标记", c.sunMarker, "用红色标记显示太阳方向在屏幕上的投影。");
    changed |= Choice("日志级别", c.logLevel, 0, kLogLevelNames, "控制 CoAVolFog.log 记录的详细程度。");
    return changed;
}
}

void SettingsPanel::PlaceWindow()
{
    const float line = ImGui::GetFontSize();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float margin = kPanelMarginInLines * line;
    const ImVec2 smallest(kMinPanelWidthInLines * line, kMinPanelHeightInLines * line);
    const ImVec2 largest(std::max(display.x - margin, smallest.x), std::max(display.y - margin, smallest.y));
    const ImVec2 firstSize(kPanelWidthInLines * line,
                           std::max(smallest.y, std::min(kPanelHeightInLines * line, display.y - 2.0f * margin)));
    const PanelPlacement& p = m_placement;
    ImGui::SetNextWindowPos(p.known ? ImVec2(p.position[0], p.position[1]) : ImVec2(margin, margin),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(p.known ? ImVec2(p.size[0], p.size[1]) : firstSize, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(smallest, largest);
}

void SettingsPanel::RememberPlacement()
{
    const ImVec2 position = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    m_placement = {true, {position.x, position.y}, {size.x, size.y}};
}

bool SettingsPanel::DrawSettings(Config& edited, const FogFrameStatus& fogStatus)
{
    const float footer =
        m_footerHeight > 0.0f ? m_footerHeight : kFirstFooterLines * ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("Settings", ImVec2(0.0f, -footer), ImGuiChildFlags_None, ImGuiWindowFlags_NoNavInputs);
    DrawStatus(fogStatus);
    const float labels =
        std::min(kLabelWidthInLines * ImGui::GetFontSize(), kLabelShareOfWidth * ImGui::GetContentRegionAvail().x);
    ImGui::PushItemWidth(-labels);
    bool changed = DrawQuality(edited);
    changed |= DrawDensity(edited);
    changed |= DrawLight(edited);
    changed |= DrawGlowAndGrading(edited);
    changed |= DrawWorld(edited);
    changed |= DrawAntialiasing(edited);
    changed |= DrawWater(edited);
    changed |= DrawDebug(edited);
    ImGui::PopItemWidth();
    ImGui::EndChild();
    return changed;
}

void SettingsPanel::DrawFooter(ConfigStore& store, const WaterFrameStatus& waterStatus,
                               const MultisamplingStatus& multisampling)
{
    const float top = ImGui::GetCursorPosY();
    ImGui::Separator();
    DrawWaterStatus(waterStatus);
    DrawMultisamplingStatus(multisampling);
    DrawSaveRow(store);
    m_footerHeight = ImGui::GetCursorPosY() - top;
}

void SettingsPanel::Draw(ConfigStore& store, const FogFrameStatus& fogStatus, const WaterFrameStatus& waterStatus,
                         const MultisamplingStatus& multisampling, bool& open)
{
    PlaceWindow();
    const bool expanded =
        ImGui::Begin("体积雾与水面###CoAVolFog", &open, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavInputs);
    if (!expanded)
    {
        ImGui::End();
        return;
    }
    RememberPlacement();
    ImGui::PushTextWrapPos(0.0f);
    Config edited = store.Get();
    if (DrawSettings(edited, fogStatus))
        store.Apply(edited);
    if (!ImGui::IsAnyItemActive())
        store.LogSettledEdits();
    DrawFooter(store, waterStatus, multisampling);
    ImGui::PopTextWrapPos();
    ImGui::End();
}

void SettingsPanel::DrawSaveRow(ConfigStore& store)
{
    const bool unsaved = store.HasUnsavedChanges();
    ImGui::BeginDisabled(!unsaved);
    if (ImGui::Button("保存"))
        m_saveFailed = !store.Save();
    ImGui::SetItemTooltip("将修改写入 CoAVolFog.ini。");
    ImGui::SameLine();
    if (ImGui::Button("还原"))
    {
        store.Revert();
        m_saveFailed = false;
    }
    ImGui::SetItemTooltip("重新读取 CoAVolFog.ini 中的设置。");
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (m_saveFailed)
        StatusLine(kErrorColour, "无法写入 CoAVolFog.ini");
    else
        StatusLine(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "%s",
                   unsaved ? "有未保存的修改" : "与 CoAVolFog.ini 一致");
}
