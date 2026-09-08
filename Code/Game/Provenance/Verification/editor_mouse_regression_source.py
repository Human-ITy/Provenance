"""Emit an isolated C++ harness using actual production method bodies.

Input/UI are stubs: these tests cover state transitions, not native Windows
focus delivery or rendering. Regenerate after editing production code.
"""
from pathlib import Path

code = Path(__file__).resolve().parents[3]

def method(path, signature):
    text = (code / path).read_text(encoding='utf-8-sig')
    start = text.index(signature)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

setter = method('Engine/Camera/Components/Component_ToolsCamera.h', 'inline void SetUpdateEnabled(')
post = method('EngineTools/Core/EditorTool.cpp', 'void EditorTool::PostDrawUpdate(')
update = method('Engine/Camera/Components/Component_ToolsCamera.cpp', 'void ToolsCameraComponent::Update(')
update_prefix = update[update.index('{')+1:update.index('// Mode')]
start_frame = method('Applications/Editor/EditorUI.cpp', 'void EditorUI::StartFrame(')
reset = 'ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;'
assert reset in start_frame, 'Shared mouse flag must be reset by EditorUI each frame'
assert start_frame.index(reset) < start_frame.index('m_fileRegistry.Update()')

print(r'''
#include <cstdio>
#include <cstdlib>
#define EE_ASSERT(x) do { if (!(x)) std::abort(); } while(0)
constexpr int ImGuiConfigFlags_NoMouse = 16;
constexpr int ImGuiMouseCursor_None = -1;
namespace ImGui {
    struct IO { int ConfigFlags = 0; bool AppFocusLost = false; } io;
    int cursor = 0;
    IO& GetIO() { return io; }
    void SetMouseCursor(int value) { cursor = value; }
}
namespace EE {
namespace Input {
    enum class InputID { Mouse_Right, Mouse_Middle, Keyboard_Escape };
    struct KeyboardMouse {
        bool right=false, middle=false, escape=false;
        bool IsHeldDown(InputID id) const {
            return id==InputID::Mouse_Right ? right : id==InputID::Mouse_Middle ? middle : escape;
        }
    } keyboard;
    struct InputSystem { KeyboardMouse* GetKeyboardMouse() { return &keyboard; } } input;
}
struct UpdateContext {
    template<class T> T* GetSystem() const { return &Input::input; }
};
struct ToolsCameraComponent {
    bool m_isUpdateEnabled=true, m_bIsManipulatingView=false;
''')
print(setter)
print('bool IsManipulatingView() const { return m_bIsManipulatingView; }')
print('void UpdateEntry() {' + update_prefix + '\n}\n};')
print(r'''
struct EditorTool {
    ToolsCameraComponent* m_pCamera;
    bool m_isViewportHovered=false;
    bool HasEntityWorld() const { return m_pCamera != nullptr; }
    void PostDrawUpdate(UpdateContext const&,bool,bool);
};
''')
print(post)
print('}\nvoid BeginFrame() { ' + reset + ' ImGui::cursor=0; }')
print(r'''
int checks=0, failures=0;
void Check(bool ok, char const* label) {
    ++checks;
    if (!ok) { ++failures; std::printf("FAIL %s\n", label); }
}
int main() {
    using namespace EE;
    UpdateContext ctx;
    ToolsCameraComponent camera;
    EditorTool tool{&camera};
    auto& k=Input::keyboard;
    // Saved dump state: disabled but manipulating, no buttons, no focus.
    camera.m_isUpdateEnabled=false; camera.m_bIsManipulatingView=true;
    ImGui::io.ConfigFlags=0x491;
    BeginFrame(); tool.PostDrawUpdate(ctx,true,false);
    Check(!camera.m_isUpdateEnabled && !camera.IsManipulatingView(),"dump state cleared");
    Check(!(ImGui::io.ConfigFlags&16) && ImGui::cursor==0,"dump restores UI");
    Check(ImGui::io.ConfigFlags==0x481,"unrelated UI flags preserved");
    // Setter cancels immediately, even if Update is never called again.
    camera.m_bIsManipulatingView=true; camera.SetUpdateEnabled(false);
    Check(!camera.IsManipulatingView(),"disable cancels immediately");
    camera.m_bIsManipulatingView=true; camera.UpdateEntry();
    Check(!camera.IsManipulatingView(),"disabled update also clears stale state");
    camera.m_bIsManipulatingView=true; camera.SetUpdateEnabled(true);
    Check(camera.IsManipulatingView(),"enabling preserves current drag");
    // Every combination of visibility/focus/hover/old manipulation/RMB/MMB/Escape/app focus.
    for(int mask=0;mask<256;++mask) {
        bool visible=mask&1,focused=mask&2,hovered=mask&4,oldDrag=mask&8;
        k.right=mask&16; k.middle=mask&32; k.escape=mask&64;
        ImGui::io.AppFocusLost=mask&128;
        tool.m_isViewportHovered=hovered;
        camera.m_isUpdateEnabled=true; camera.m_bIsManipulatingView=oldDrag;
        BeginFrame(); tool.PostDrawUpdate(ctx,visible,focused);
        bool eligible=visible&&focused&&!k.escape&&!ImGui::io.AppFocusLost;
        bool drag=eligible&&oldDrag&&(k.right||k.middle);
        Check(camera.m_isUpdateEnabled==(eligible&&(hovered||drag)),"eligibility matrix");
        Check(bool(ImGui::io.ConfigFlags&16)==drag,"suppression matrix");
        Check(camera.m_isUpdateEnabled||!camera.IsManipulatingView(),"disabled invariant");
    }
    // Releasing a drag while hover is suppressed must recover next frame.
    ImGui::io.AppFocusLost=false; k={}; k.right=true;
    camera.m_isUpdateEnabled=true; camera.m_bIsManipulatingView=true;
    tool.m_isViewportHovered=false;
    BeginFrame(); tool.PostDrawUpdate(ctx,true,true);
    Check(camera.m_isUpdateEnabled && (ImGui::io.ConfigFlags&16),"drag survives no hover");
    k.right=false;
    BeginFrame(); tool.PostDrawUpdate(ctx,true,true);
    Check(!camera.IsManipulatingView() && !(ImGui::io.ConfigFlags&16),"release unlocks UI");
    tool.m_isViewportHovered=true;
    BeginFrame(); tool.PostDrawUpdate(ctx,true,true);
    Check(camera.m_isUpdateEnabled,"hover reacquires camera after release");
    // Focus loss during held RMB; refocus must not revive old suppression.
    k.right=true; camera.m_bIsManipulatingView=true;
    BeginFrame(); tool.PostDrawUpdate(ctx,true,false);
    Check(!camera.IsManipulatingView() && !(ImGui::io.ConfigFlags&16),"focus loss cancels drag");
    BeginFrame(); tool.PostDrawUpdate(ctx,true,true);
    Check(camera.m_isUpdateEnabled && !(ImGui::io.ConfigFlags&16),"refocus has no stale drag");
    // Multiple tools may request, but inactive ones cannot revoke others' request.
    ToolsCameraComponent inactive;
    EditorTool other{&inactive};
    for (int order=0;order<2;++order) {
        camera.m_bIsManipulatingView=true; inactive.m_bIsManipulatingView=true;
        BeginFrame();
        if(order) other.PostDrawUpdate(ctx,true,false);
        tool.PostDrawUpdate(ctx,true,true);
        if(!order) other.PostDrawUpdate(ctx,true,false);
        Check(bool(ImGui::io.ConfigFlags&16),"tool order preserves active request");
        Check(!inactive.IsManipulatingView(),"inactive tool cancels");
    }
    BeginFrame(); // Closing the last tool must not strand suppression.
    Check(!(ImGui::io.ConfigFlags&16),"no surviving tool releases suppression");
    std::printf("Mouse state regression: %d checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
''')
