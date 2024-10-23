#include "GameEngineCore/Window.hpp"
#include "GameEngineCore/Debug.hpp"
#include "GameEngineCore/Rendering/OpenGL/ShaderProgram.hpp"
#include "GameEngineCore/Rendering/OpenGL/VertexBuffer.hpp"
#include "GameEngineCore/Rendering/OpenGL/VertexArray.hpp"
#include "GameEngineCore/Rendering/OpenGL/IndexBuffer.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/backends/imgui_impl_glfw.h>

namespace GameEngine
{
    GLfloat positions_colors2[] = {
        -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f,
        0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f,
        0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f};

    GLuint indices[] = {
        0, 1, 2, 3, 2, 1};

    const char *vertex_shader =
        "#version 460\n"
        "layout(location = 0) in vec3 vertex_position;"
        "layout(location = 1) in vec3 vertex_color;"
        "out vec3 color;"
        "void main() {"
        "   color = vertex_color;"
        "   gl_Position = vec4(vertex_position, 1.0);"
        "}";

    const char *fragment_shader =
        "#version 460\n"
        "in vec3 color;"
        "out vec4 frag_color;"
        "void main() {"
        "   frag_color = vec4(color, 1.0);"
        "}";

    std::unique_ptr<ShaderProgram> p_shader_program;
    std::unique_ptr<VertexBuffer> p_positions_colors_vbo;
    std::unique_ptr<IndexBuffer> p_index_buffer;
    std::unique_ptr<VertexArray> p_vao;

    static bool s_GLfW_initialized = false;

    Window::Window(std::string title, const unsigned int width, const unsigned int height)
        : m_data({std::move(title), width, height})
    {
        int resultCode = init();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init();
        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    }

    Window::~Window()
    {
        shutdown();
    }

    int Window::init()
    {
        if (!s_GLfW_initialized)
        {
            if (!glfwInit())
            {
                LOG_CRITICAL("Failed to initialize GLFW");
                return -1;
            }
            s_GLfW_initialized = true;
        }

        m_window = glfwCreateWindow(m_data.width, m_data.height, m_data.title.c_str(), nullptr, nullptr);
        if (!m_window)
        {
            LOG_CRITICAL("Failed to create window");
            glfwTerminate();
            return -2;
        }

        glfwMakeContextCurrent(m_window);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            LOG_CRITICAL("Failed to initialize GLAD");
            return -3;
        }

        glfwSetWindowUserPointer(m_window, &m_data);

        glfwSetWindowSizeCallback(m_window,
                                  [](GLFWwindow *window, int width, int height)
                                  {
                                      WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
                                      data.height = height;
                                      data.width = width;

                                      EventWindowResized event(width, height);
                                      data.event_callback(event);
                                  });

        glfwSetCursorPosCallback(m_window,
                                 [](GLFWwindow *window, double x, double y)
                                 {
                                     WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));

                                     EventMouseMoved event(x, y);
                                     data.event_callback(event);
                                 });

        glfwSetWindowCloseCallback(m_window,
                                   [](GLFWwindow *window)
                                   {
                                       WindowData &data = *static_cast<WindowData *>(glfwGetWindowUserPointer(window));
                                       EventWindowClose event;
                                       data.event_callback(event);
                                   });

        glfwSetFramebufferSizeCallback(m_window,
                                       [](GLFWwindow *window, int width, int height)
                                       {
                                           glViewport(0, 0, width, height);
                                       });

        p_shader_program = std::make_unique<ShaderProgram>(vertex_shader, fragment_shader);
        if (!p_shader_program->isCompiled())
        {
            return false;
        }

        BufferLayout buffer_layout_1vec3{
            ShaderDataType::Float3};

        BufferLayout buffer_layout_2vec3{
            ShaderDataType::Float3,
            ShaderDataType::Float3};

        p_vao = std::make_unique<VertexArray>();
        p_positions_colors_vbo = std::make_unique<VertexBuffer>(positions_colors2, sizeof(positions_colors2), buffer_layout_2vec3);
        p_index_buffer = std::make_unique<IndexBuffer>(indices, sizeof(indices) / sizeof(GLuint));

        p_vao->add_vertex_buffer(*p_positions_colors_vbo);
        p_vao->set_index_buffer(*p_index_buffer);
        return 0;
    }

    void Window::on_update()
    {
        glClearColor(m_background_color[0], m_background_color[1], m_background_color[2], m_background_color[3]);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize.x = static_cast<float>(get_width());
        io.DisplaySize.y = static_cast<float>(get_height());

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Triangle color window
        ImGui::Begin("Triangle color");
        // ImGui::ColorEdit3("color1", positions_colors + 3);
        // ImGui::ColorEdit3("color2", positions_colors + 9);
        // ImGui::ColorEdit3("color3", positions_colors + 15);

        p_shader_program->bind();
        p_vao->bind();
        // p_positions_colors_vbo->update_buffer(positions_colors, sizeof(positions_colors));
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(p_vao->get_indices_count()), GL_UNSIGNED_INT, nullptr);

        ImGui::End();
        //

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
        glfwPollEvents();
    }

    void Window::shutdown()
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
}