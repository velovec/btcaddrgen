pipeline {
    agent {
        docker {
            image 'ubuntu:20.04'
            label 'docker'
            args '-v /usr/bin/docker:/usr/bin/docker -v /var/run/docker.sock:/var/run/docker.sock -v ${HOME}/.docker:${WORKSPACE}/.docker'
        }
    }

    environment {
        DOCKER_REPOSITORY = 'docker.velovec.pro/btc'
        DOCKER_CONFIG = "${env.WORKSPACE}/.docker"
    }

    options {
        buildDiscarder(logRotator(numToKeepStr: '15', daysToKeepStr: '30'))
    }

    stages {
        stage ('Build :: Build Artifacts') {
            steps {
                sh "docker build -t ${env.DOCKER_REPOSITORY}/btcaddrgen:latest ."
            }
        }

        stage ('Promote') {
            when {
                branch 'master'
            }

            steps {
                sh "docker push ${env.DOCKER_REPOSITORY}/btcaddrgen:latest"
            }
        }
    }

    post {
        always {
            sh 'docker rmi $(docker images | grep btcaddrgen:latest | awk \'{ print $3 }\')'

            deleteDir()
        }
    }
}